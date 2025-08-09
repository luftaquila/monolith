<script setup>
  defineOptions({name: 'Telemetry'});

  import {ref, onMounted} from 'vue';
  import {dark} from '@/layout/composables/layout';
  import {publish} from '@/service/mqtt';
  import {term} from '@/service/terminal';
  import {state, times, cons, telemetry, fmt, digit} from '@/service/state';
  import {views, units, colors} from '@/service/ui';
  import {map, line, path, speed, course, attitude} from '@/service/telemetry';
  import {init_map} from '@/service/map';

  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';

  import "@xterm/addon-fit";
  import "@xterm/xterm/css/xterm.css";

  import dayjs from 'dayjs/esm';

  const terminal = ref(null);
  const container = {
    state: ref(null),
    analog: ref(null),
    gyro: ref(null),
    gps: ref(null),
  };

  onMounted(() => {
    const fit = new FitAddon.FitAddon();
    term.loadAddon(fit);
    term.open(terminal.value);
    fit.fit();
    window.addEventListener('resize', () => fit.fit());

    if (!window.kakao) {
      const script = document.createElement("script");
      script.type = 'text/javascript';
      script.src = "//dapi.kakao.com/v2/maps/sdk.js?appkey=5a6908c6e8974084c9c219f330401972&autoload=false";
      script.onload = () => init_map(map, line, path, container.gps);
      document.head.appendChild(script);
    } else {
      init_map(map, line, path, container.gps);
    }

    init_chart();

    attitude.value.loadBodyFromGLTF('shuttle.glb', {
      scale: 0.5,
      centerXY: true,
      keepArrow: true,
      floorGap: 0,
      placeOnFloor: true
    });
  });

  const axis = {
    temp: {splits: [], min: 0, max: 0},
    accel: {splits: [], min: 0, max: 0},
    gyro: {splits: [], min: 0, max: 0},
  };

  function init_chart() {
    if (telemetry.chart.analog || telemetry.chart.gyro) {
      telemetry.chart.analog?.destroy();
      telemetry.chart.gyro?.destroy();
    }

    const scales = {};
    const axes = [{
      size: 35,
      values: (u, v) => v.map(x => dayjs(x * 1000).format('HH:mm:ss')),
      stroke: () => dark.value ? '#fff' : '#000',
      ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
      grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
    }];

    for (const [i, [k, o]] of Object.entries(units).entries()) {
      axis[k] = {splits: [], min: 0, max: 0};

      scales[k] = {
        range: (u, d_min, d_max) => {
          if (d_min === null && d_max === null) {
            return [null, null];
          } else {
            axis[k] = split_range(d_min, d_max);
            return [axis[k].min, axis[k].max];
          }
        }
      };

      axes.push({
        scale: k,
        side: i % 2 ? 1 : 3,
        size: 50 + (o.unit.length - 1) * 5,
        values: (u, v) => v.map(x => `${digit(x)}${o.unit}`),
        splits: () => axis[k].splits,
        stroke: () => dark.value ? '#fff' : '#000',
        ticks: {stroke: () => dark.value ? '#24282b' : '#ededed'},
        grid: {stroke: () => dark.value ? '#24282b' : '#ededed'},
      });

      fmt[k] = (u, v, sidx, didx) => {
        const d = u.data[sidx];

        if (didx == null && d) {
          v = d[d.length - 1];
        }

        if (isNaN(v) || v === undefined || v === null) {
          return '-';
        } else {
          return `${digit(v)} ${o.unit}`;
        }
      };
    }

    telemetry.chart.analog = new uPlot({
      width: 600, height: 400,
      pxAlign: 0, pxSnap: false,
      scales: scales,
      series: [
        {value: fmt.time},
        {label: views.analog.ch.ain1.name, stroke: colors[0], value: fmt[views.analog.ch.ain1.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain1.unit || 'Volt'},
        {label: views.analog.ch.ain2.name, stroke: colors[1], value: fmt[views.analog.ch.ain2.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain2.unit || 'Volt'},
        {label: views.analog.ch.ain3.name, stroke: colors[2], value: fmt[views.analog.ch.ain3.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain3.unit || 'Volt'},
        {label: views.analog.ch.ain4.name, stroke: colors[3], value: fmt[views.analog.ch.ain4.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain4.unit || 'Volt'},
        {label: views.analog.ch.ain5.name, stroke: colors[4], value: fmt[views.analog.ch.ain5.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain5.unit || 'Volt'},
        {label: views.analog.ch.ain6.name, stroke: colors[5], value: fmt[views.analog.ch.ain6.unit], points: {show: false}, pxAlign: 0, scale: views.analog.ch.ain6.unit || 'Volt'},
        {label: views.analog.ch.volt.name, stroke: colors[6], value: fmt.Volt, points: {show: false}, pxAlign: 0, scale: 'Volt'},
        {label: views.analog.ch.temp.name, stroke: colors[7], value: fmt.Temperature, points: {show: false}, pxAlign: 0, scale: 'Temperature', show: false},
      ],
      axes: axes,
    }, telemetry.analog, container.analog.value);

    telemetry.chart.gyro = new uPlot({
      width: 600, height: 400,
      pxAlign: 0, pxSnap: false,
      scales: scales,
      series: [
        {value: fmt.time},
        {label: "Ax", stroke: colors[0], value: fmt.Acceleration, points: {show: false}, pxAlign: 0, scale: 'Acceleration'},
        {label: "Ay", stroke: colors[1], value: fmt.Acceleration, points: {show: false}, pxAlign: 0, scale: 'Acceleration'},
        {label: "Az", stroke: colors[2], value: fmt.Acceleration, points: {show: false}, pxAlign: 0, scale: 'Acceleration'},
        {label: "Gx", stroke: colors[3], value: fmt['Angular Velocity'], points: {show: false}, pxAlign: 0, scale: 'Angular Velocity'},
        {label: "Gy", stroke: colors[4], value: fmt['Angular Velocity'], points: {show: false}, pxAlign: 0, scale: 'Angular Velocity'},
        {label: "Gz", stroke: colors[5], value: fmt['Angular Velocity'], points: {show: false}, pxAlign: 0, scale: 'Angular Velocity'},
      ],
      axes: axes,
    }, telemetry.gyro, container.gyro.value);

    setInterval(() => {
      Object.entries(telemetry.chart).forEach(e => {
        telemetry.chart[e[0]].setScale('x', {
          min: new Date().getTime() / 1000 - 60,
          max: new Date().getTime() / 1000
        });
      });
    }, 100);

    new ResizeObserver(entries => {
      for (let entry of entries) {
        Object.entries(telemetry.chart).forEach(e => {
          telemetry.chart[e[0]].setSize({
            width: entry.contentRect.width,
            height: entry.contentRect.width * 0.6
          });
        });
      }
    }).observe(container.state.value);
  }

  function split_range(d_min, d_max) {
    if (d_min === d_max) {
      d_min *= 0.85;
      d_max *= 1.15;
    }

    const tick = 5;
    const step = (d_max - d_min) / (tick - 1);
    const min = Math.floor(d_min / step) * step;
    const max = min + step * tick;
    const splits = Array.from({length: tick + 1}, (_, i) => min + i * step);
    return {min, max, splits};
  }

  function geolocation() {
    if (map) {
      navigator.geolocation.getCurrentPosition((pos) => {
        map.value.setCenter(new kakao.maps.LatLng(pos.coords.latitude, pos.coords.longitude));
      });
    }
  }

  function send_usrevt() {
    cons.usrevt = cons.usrevt.replace(/[^\x20-\x7E]/g, '').trim().slice(0, 16);

    if (cons.usrevt.length === 0) {
      cons.usrevt = 'USREVT';
    }

    publish('cmd/evt', cons.usrevt, 1);
  }

  function send_can() {
    // TODO:
  }

  function ascii_only(event) {
    if (!/^[\x20-\x7E]*$/.test(event.target.value)) {
      event.target.value = event.target.value.replace(/[^\x20-\x7E]/g, '');
    }
  }

  function hex_only(event) {
    if (!/^[0-9A-Fa-fx]*$/.test(event.target.value)) {
      event.target.value = event.target.value.replace(/[^0-9A-Fa-fx]/g, '');
    }
  }
</script>

<template>
  <div class="grid grid-cols-12 gap-8">
    <div class="col-span-full lg:col-span-12">
      <div class="card" :ref="container.state">
        <div class="font-semibold text-xl mb-6">System State</div>
        <div v-for="(tag, key) in times" :key="key" class="flex items-center mb-6">
          <span class="w-24 font-medium">{{ tag.label }}</span>
          <Tag :value="tag.value" severity="info" class="timetag" />
        </div>
        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 xl:grid-cols-5 gap-4 text-sm">
          <template v-for="item in state" :key="item.name">
            <div v-if="item.name" class="flex items-center cardview">
              <span class="w-full ">{{ item.name }}</span>
              <Tag :value="item.text" :severity="item.status" class="ml-2 state" />
            </div>
          </template>
        </div>
      </div>

      <div v-if="views.digital.display.telemetry" class="card">
        <div class="font-semibold text-xl mb-6">Digital</div>
        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4">
          <template v-for="item in views.digital.ch" :key="item.name">
            <div v-if="item.name" class="flex items-center cardview">
              <span class="w-full">{{ item.name }}</span>
              <Tag :value="item.value ? 'HIGH' : 'LOW'" :severity="item.value ? 'info' : 'danger'" />
            </div>
          </template>
        </div>
      </div>

      <div v-if="views.analog.display.telemetry" class="card">
        <div class="font-semibold text-xl mb-4">Analog</div>
        <div class="chart" :ref="container.analog"></div>
      </div>

      <div v-if="views.gyro.display.telemetry" class="card">
        <div class="font-semibold text-xl mb-4">Gyroscope</div>
        <div class="chart" :ref="container.gyro"></div>
        <Attitude3D ref="attitude" :show-grid="true" />
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">CAN</div>
      </div>

      <div v-if="views.gps.display.telemetry" class="card" style="position: relative;">
        <div class="font-semibold text-xl mb-6">GPS</div>
        <div class="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4 mb-6">
          <div class="flex items-center">
            <span class="w-20 font-medium">Speed</span>
            <Tag :value="speed" severity="info" class="ml-2 state timetag" />
          </div>
          <div class="flex items-center">
            <span class="w-20 font-medium">Course</span>
            <Tag :value="course" severity="info" class="ml-2 state timetag" />
          </div>
        </div>

        <div class="relative">
          <div :ref="container.gps" style="width: 100%; aspect-ratio: 1 / 0.7;"></div>
          <Button class="!absolute bottom-3 right-3 z-10 h-10" severity="secondary" icon="pi pi-map-marker"
            @click="geolocation" />
        </div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">Console</div>
        <div class="mb-6">
          <label>Transmit User Event</label>
          <InputGroup class="mt-4 mb-3">
            <InputText v-model="cons.usrevt" placeholder="(default: USREVT)" maxlength="16" @keyup="ascii_only" />
            <Button icon="pi pi-send" @click="send_usrevt" />
          </InputGroup>
          <Message size="small" severity="secondary" variant="simple">Only ASCII characters up to 16 bytes.</Message>
        </div>
        <div>
          <label>Transmit CAN Message</label>
          <InputGroup class="mt-4 mb-2">
            <InputText v-model="cons.can.id" placeholder="CAN Message ID" maxlength="10" @keyup="hex_only" />
            <Button icon="pi pi-send" @click="send_can" />
          </InputGroup>
          <InputGroup class="mt-4 mb-3">
            <InputText v-model="cons.can.data[n - 1]" v-for="n in 8" :key="n" :placeholder="`D${n - 1}`" maxlength="2"
              @keyup="hex_only" class="can_data" />
          </InputGroup>
          <Message size="small" severity="secondary" variant="simple">CAN msg ID and data bytes in HEX format.</Message>
        </div>
      </div>

      <div class="card">
        <div class="font-semibold text-xl mb-6">System Events</div>
        <div ref="terminal" class="text-sm"></div>
      </div>
    </div>
  </div>
</template>

<style>
  .xterm {
    padding: 0.5rem 0.7rem;
    border-radius: 1rem;
    height: 20rem;
  }

  .xterm-viewport {
    border-radius: 0.5rem;
    height: 100%;
  }

  .state .p-tag-label {
    font-size: 0.75rem;
  }

  .timetag .p-tag-label {
    font-size: 0.95rem;
  }

  .can_data {
    font-size: 0.9rem !important;
    height: 2.25rem;
  }
</style>
