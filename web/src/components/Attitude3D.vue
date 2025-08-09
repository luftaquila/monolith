<template>
  <div ref="container" class="attitude3d-root"></div>
</template>

<script setup>
  import {onMounted, onBeforeUnmount, ref} from 'vue'
  import * as THREE from 'three'
  import {OrbitControls} from 'three/examples/jsm/controls/OrbitControls.js'
  import {GLTFLoader} from 'three/examples/jsm/loaders/GLTFLoader.js'

  const props = defineProps({
    tau: {type: Number, default: 0.5},     // accel 보정 강도(초)
    fov: {type: Number, default: 60},
    showGrid: {type: Boolean, default: true},
    showAxes: {type: Boolean, default: true},
    background: {type: [Number, String], default: 0x141a22}
  })

  const container = ref(null)

  // three.js 기본
  let scene, camera, renderer, controls, grid, axes, hemi, dirLight
  let ro, rafId = 0

  // 본체를 담는 계층: bodyRoot -> bodyPivot -> bodyMesh
  // - bodyRoot : 월드에서 우리가 회전시킬 노드 (자세 쿼터니언을 여기에 적용)
  // - bodyPivot: 모델 축 보정/스케일/오프셋을 적용하는 노드
  // - bodyMesh : 실제 모델(큐브 기본값 또는 GLTF 내용)
  const bodyRoot = new THREE.Group()
  const bodyPivot = new THREE.Group()
  let bodyMesh = null
  let forwardArrow = null

  // 필터 상태
  let q = new THREE.Quaternion()
  let lastTs = null
  const eulerTmp = new THREE.Euler()
  const accelQ = new THREE.Quaternion()
  const accelVec = new THREE.Vector3()

  function resize() {
    if (!container.value || !renderer || !camera) return
    const w = container.value.clientWidth || 1
    const h = container.value.clientHeight || 1
    camera.aspect = w / h
    camera.updateProjectionMatrix()
    renderer.setSize(w, h, false)
  }

  function animate() {
    controls?.update()
    renderer?.render(scene, camera)
    rafId = requestAnimationFrame(animate)
  }

  function setupScene() {
    scene = new THREE.Scene()
    scene.background = typeof props.background === 'number'
      ? new THREE.Color(props.background)
      : new THREE.Color(props.background || '#141a22')
    scene.up.set(0, 0, 1) // Z-up

    camera = new THREE.PerspectiveCamera(props.fov, 1, 0.01, 100)
    camera.position.set(0, -3, 1)
    camera.up.set(0, 0, 1)

    renderer = new THREE.WebGLRenderer({antialias: true})
    renderer.setPixelRatio(window.devicePixelRatio || 1)
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.2;
    container.value.appendChild(renderer.domElement)

    controls = new OrbitControls(camera, renderer.domElement)
    controls.enableDamping = true

    if (props.showAxes) {
      axes = new THREE.AxesHelper(3) // X=red, Y=green, Z=blue
      scene.add(axes)
    }

    // 조명
    hemi = new THREE.HemisphereLight(0xffffff, 0x111111, 1)
    scene.add(hemi)
    dirLight = new THREE.DirectionalLight(0xffffff, 1.0)  // Key
    dirLight.position.set(3, 3, 2)
    scene.add(dirLight)
    const fill = new THREE.DirectionalLight(0xffffff, 0.5) // Fill
    fill.position.set(-2, 1, 1)
    scene.add(fill)
    const rim = new THREE.DirectionalLight(0xffffff, 0.6)  // Rim/Back
    rim.position.set(-1, -3, 2)
    scene.add(rim)

    // 본체 계층 추가
    scene.add(bodyRoot)
    bodyRoot.add(bodyPivot)

    // 기본 큐브를 본체로 설정
    setBody(makeDefaultCube(), {keepArrow: true})

    // 리사이즈 + 루프
    ro = new ResizeObserver(resize)
    ro.observe(container.value)
    resize()
    animate()
  }

  function dispose() {
    cancelAnimationFrame(rafId)
    ro?.disconnect()
    controls?.dispose()
    renderer?.dispose()

    // 메쉬 정리
    clearBody()

    if (renderer?.domElement && container.value?.contains(renderer.domElement)) {
      container.value.removeChild(renderer.domElement)
    }
  }

  /* ============ 본체 바꾸기 API ============ */

  /** 기본 큐브 메쉬 생성(+Y 화살표는 setBody에서 붙임) */
  function makeDefaultCube() {
    const mesh = new THREE.Mesh(
      new THREE.BoxGeometry(1, 0.6, 0.1),
      new THREE.MeshStandardMaterial({color: 0x94a3b8, metalness: 0.1, roughness: 0.6})
    )
    mesh.castShadow = mesh.receiveShadow = true
    return mesh
  }

  /** 현재 본체 제거 */
  function clearBody() {
    if (forwardArrow) {
      forwardArrow.parent?.remove(forwardArrow)
      forwardArrow = null
    }
    if (bodyMesh) {
      // 메쉬/리소스 해제
      bodyMesh.traverse(obj => {
        if (obj.isMesh) {
          obj.geometry?.dispose?.()
          if (Array.isArray(obj.material)) obj.material.forEach(m => m.dispose?.())
          else obj.material?.dispose?.()
        }
      })
      bodyPivot.remove(bodyMesh)
      bodyMesh = null
    }
  }

  /**
   * 본체로 사용할 객체 설정
   * @param {THREE.Object3D} object3D
   * @param {Object} opts
   *   - scale:number=1
   *   - offset:{x,y,z}={0,0,0}
   *   - euler:{roll,pitch,yaw} (deg)  // 모델 축 보정
   *   - center:boolean=true           // bbox 중심을 원점으로 이동
   *   - keepArrow:boolean=true        // +Y 화살표 표시
   */
  function setBody(object3D, opts = {}) {
    const {
      scale = 1,
      offset = {x: 0, y: 0, z: 0},
      euler = {roll: 0, pitch: 0, yaw: 0},
      center = true,
      keepArrow = true
    } = opts

    clearBody()

    bodyMesh = object3D
    bodyMesh.traverse(o => (o.castShadow = o.receiveShadow = true))

    // 중심 맞추기
    if (center) {
      const box = new THREE.Box3().setFromObject(bodyMesh)
      const centerV = new THREE.Vector3()
      box.getCenter(centerV)
      bodyMesh.position.sub(centerV) // 모델 내부 원점을 bbox 중심으로 이동
    }

    // 스케일
    bodyMesh.scale.setScalar(scale)

    // 모델 축 보정 (roll=X, pitch=Y, yaw=Z in deg)
    const r = THREE.MathUtils.degToRad(euler.roll || 0)
    const p = THREE.MathUtils.degToRad(euler.pitch || 0)
    const y = THREE.MathUtils.degToRad(euler.yaw || 0)
    bodyMesh.rotation.set(r, p, y, 'XYZ')

    // 오프셋
    bodyMesh.position.add(new THREE.Vector3(offset.x || 0, offset.y || 0, offset.z || 0))

    bodyPivot.add(bodyMesh)

    // 진행방향(+Y) 화살표
    if (keepArrow) {
      forwardArrow = new THREE.ArrowHelper(
        new THREE.Vector3(0, 1, 0),
        new THREE.Vector3(0, 0, 0),
        0.6, 0x3b82f6, 0.12, 0.08
      )
      bodyPivot.add(forwardArrow)
    }
  }

  /**
   * GLTF/GLB 로드 후 본체로 설정
   * @param {string} url
   * @param {Object} opts (setBody와 동일 옵션)
   */
  async function loadBodyFromGLTF(url, opts = {}) {
    const loader = new GLTFLoader()
    return new Promise((resolve, reject) => {
      loader.load(
        url,
        (gltf) => {
          // glTF는 보통 Y-up. 이 컴포넌트는 씬 Z-up이지만
          // Object3D.up은 lookAt에만 영향 -> 여기선 수동 축보정으로 처리.
          const root = gltf.scene || gltf.scenes?.[0]
          if (!root) {reject(new Error('GLTF scene not found')); return }
          setBody(root, opts)
          resolve(root)
        },
        undefined,
        (err) => reject(err)
      )
    })
  }

  /* ============ IMU 입력 (pushImu 유지) ============ */
  /**
   * sample = {
   *   ts: Number(초),
   *   gyro:  { x: dps, y: dps, z: dps },  // Z-up, Y-forward 기준 바디 프레임
   *   accel: { x: g,   y: g,   z: g   }   // 중력 포함
   * }
   */
  function imu(sample) {
    const ts = sample.ts
    if (lastTs == null) {lastTs = ts; return }
    const dt = Math.max(1e-6, ts - lastTs)
    lastTs = ts

    // 1) yaw는 자이로 적분
    const wx = THREE.MathUtils.degToRad(sample.gyro?.x ?? 0)
    const wy = THREE.MathUtils.degToRad(sample.gyro?.y ?? 0)
    const wz = THREE.MathUtils.degToRad(sample.gyro?.z ?? 0)
    const dq = new THREE.Quaternion(wx * 0.5 * dt, wy * 0.5 * dt, wz * 0.5 * dt, 1).normalize()
    q.multiply(dq).normalize()

    // 2) roll/pitch는 가속도 기반 보정
    accelVec.set(sample.accel?.x ?? 0, sample.accel?.y ?? 0, sample.accel?.z ?? 0)
    if (accelVec.lengthSq() > 1e-8) {
      accelVec.normalize()
      const ax = accelVec.x, ay = accelVec.y, az = accelVec.z
      const roll = Math.atan2(ay, az)
      const pitch = Math.atan2(-ax, Math.sqrt(ay * ay + az * az))

      eulerTmp.setFromQuaternion(q, 'ZYX') // yaw 유지
      const yaw = eulerTmp.z
      eulerTmp.set(roll, pitch, yaw, 'XYZ')
      accelQ.setFromEuler(eulerTmp)

      const alpha = 1 - Math.exp(-dt / (props.tau || 0.5))
      q.slerp(accelQ, alpha).normalize()
    }

    // 3) 본체 회전 적용 (bodyRoot에 적용해 모델 교체와 독립)
    bodyRoot.quaternion.copy(q)
  }

  /* ============ 라이프사이클 ============ */
  onMounted(setupScene)
  onBeforeUnmount(dispose)

  /* 부모에서 사용할 메서드 노출 */
  defineExpose({
    imu,
    setBody,
    clearBody,
    loadBodyFromGLTF
  })
</script>

<style scoped>
  .attitude3d-root {
    width: 100%;
    height: 400px;
    /* 필요시 부모에서 override */
    display: block;
    position: relative;
  }
</style>
