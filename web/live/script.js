if (typeof io === typeof undefined) {
  Swal.fire({
    icon: 'error',
    title: '서버 응답 없음',
    html: `소켓 서버가 응답하지 않습니다.`,
    confirmButtonText: '확인',
    customClass: {
      confirmButton: 'btn green',
    }
  });
}

const timeout = 3000;
let timer = setTimeout(() => { $("#telemetry i").css("color", "red"); }, timeout);

/************************************************************************************
 * Socket.io events
 ***********************************************************************************/
socket = io.connect("/", {
  query: {
    client: true,
    channel: localStorage.getItem('id'),
    key: localStorage.getItem('key')
  }
});

socket.on('connect', () => {
  connect_time = new Date();
  $("#server i").css("color", "green");
});

socket.on('connect_error', () => {
  $("#server i").css("color", "red");
});

socket.on('disconnect', () => {
  $("#server i").css("color", "red");

  // only if event is at page load
  if (new Date() - connect_time < 1000) {
    // no saved car id cookies
    if (!localStorage.getItem('id') && !localStorage.getItem('key')) {
      Swal.fire({
        icon: 'info',
        title: '차량 ID 정보 없음',
        html: '<div style="line-height: 2.5rem;">차량 ID가 설정되지 않았습니다.<br>먼저 차량 ID와 key를 설정해 주세요.</div>',
        confirmButtonText: '확인',
        customClass: {
          confirmButton: 'btn green',
        }
      });
    } else { // invalid car id cookies
      Swal.fire({
        icon: 'error',
        title: '서버 연결 비활성화',
        html: '<div style="line-height: 2.5rem;">서버에서 연결을 거부했습니다.<br>차량 ID와 key 설정을 확인하세요.</div>',
        confirmButtonText: '확인',
        customClass: {
          confirmButton: 'btn green',
        }
      });
    }
  }
});

socket.on('socket-lost', () => {
  $("#telemetry i").css("color", "red");
});

// update UI on telemetry report
socket.on('report', data => {
  $('#timestamp').text(data.data.timestamp);
  $('#telemetry i').css('color', 'green');
  clearTimeout(timer);
  timer = setTimeout(() => { $("#telemetry i").css("color", "red"); }, timeout);

  let watchlists = watchlist[data.data.key];
  if (watchlists) {
    for (let watch of watchlists) {
      update_display(watch, data.data.parsed);
    }
  }
});

function update_display(target, data) {
  if (target.type === 'can') {
    data = parse_CAN(target.source, data);
    data *= target.scale;
  } else if (target.display !== 'gps') {
    data = data[target.parsed];
    if (target.scale) {
      data *= target.scale;
    }
  }

  switch (target.display) {
    case 'digital': {
      $(`#data_val_${target.id}`).text(data ? 'ON' : 'OFF');

      $(`#icon_${target.id}`).css('color', 'green');
      clearTimeout(timers[target.id]);
      timers[target.id] = setTimeout(() => { $(`#icon_${target.id}`).css("color", "red"); }, timeout);
      break;
    }

    case 'value': {
      let val = data.toFixed(Math.abs(data) < 10 ? 2 : 1);
      $(`#data_val_${target.id}`).text((val !== '0.00') ? val : 0);

      $(`#icon_${target.id}`).css('color', 'green');
      clearTimeout(timers[target.id]);
      timers[target.id] = setTimeout(() => { $(`#icon_${target.id}`).css("color", "red"); }, timeout);
      break;
    }

    case 'graph': {
      let val = data.toFixed(Math.abs(data) < 10 ? 2 : 1);
      $(`#data_val_${target.id}`).text((val !== '0.00') ? val : 0);
      graphs[target.id].data.push({ x: new Date(), y: data });

      $(`#icon_${target.id}`).css('color', 'green');
      clearTimeout(timers[target.id]);
      timers[target.id] = setTimeout(() => { $(`#icon_${target.id}`).css("color", "red"); }, timeout);
      break;
    }

    case 'gps': {
      let pos = new kakao.maps.LatLng(data.lat, data.lon);
      maps[target.id].path.push(pos);
      maps[target.id].line.setPath(maps[target.id].path);
      maps[target.id].map.panTo(pos);
      break;
    }
  }
}

/************************************************************************************
 * CAN data parser
 ***********************************************************************************/
function parse_CAN(source, data) {
  // TODO
  return null;
}

/************************************************************************************
 * UI drawer
 ***********************************************************************************/
ui = localStorage.getItem('ui');
graphs = { };
maps = { };
watchlist = { };
timers = { };

if (ui) {
  let json;

  try {
    json = JSON.parse(ui);
  } catch (e) {
    Swal.fire({
      icon: 'error',
      title: '잘못된 UI 설정 파일',
      html: '<div style="line-height: 2.5rem;">UI 설정을 해석할 수 없습니다.<br>UI를 새로 설정해 주세요.</div>',
      confirmButtonText: '확인',
      customClass: {
        confirmButton: 'btn green',
      }
    });
  }

  // draw ui
  for (const [ gid, group ] of json.entries()) {
    $('#dataarea').append(create_html('ui_datagroup', { name: group.name, icon: group.icon, id: gid }));

    for (const [ did, data ] of group.data.entries()) {
      const id = `${gid}_${did}`;

      timers[id] = setTimeout(() => { $(`#icon_${id}`).css("color", "red"); }, timeout);

      $(`#group_table_${gid}`).append(create_html('ui_data', {
        id: id,
        name: data.name,
        icon: data.icon,
        display: data.display,
        scale: data.scale,
        unit: data.unit,
        type: data.type,
        source: data.source,
      }));

      // draw map on gps display
      if (data.display === 'gps') {
        maps[`${id}`] = {
          map: new kakao.maps.Map(document.getElementById(`map_${id}`), {
            center: new kakao.maps.LatLng(37.2829317, 127.0435822)
          }),
          path: [],
          line: new kakao.maps.Polyline({
            strokeWeight: 5,
            strokeColor: '#00C40D',
            strokeOpacity: 0.8,
            strokeStyle: 'solid'
          })
        };

        maps[`${id}`].line.setPath(maps[`${id}`].path);
        maps[`${id}`].line.setMap(maps[`${id}`].map);

        continue; // skip to the next one
      }

      // draw graph
      if (data.display === 'graph') {
        graphs[`${id}`] = {
          data: [],
          chart: null
        }

        graphs[`${id}`].chart = new Chart($(`#graph_${id}`), {
          type: 'line',
          data: {
            datasets: [{
              data: graphs[`${id}`].data,
              cubicInterpolationMode: 'monotone',
              tension: 0.2,
              borderColor: 'rgb(54, 162, 235)'
            }]
          },
          options: {
            responsive: true,
            interaction: { intersect: false, },
            scales: {
              x: {
                type: 'realtime',
                distribution: 'linear',
                time: {
                  unit: 'second',
                  unitStepSize: 15,
                  stepSize: 15,
                  displayFormats: {
                    hour: 'H:mm:ss',
                    minute: 'H:mm:ss',
                    second: 'H:mm:ss'
                  }
                },
                realtime: {
                  duration: 60000,
                  refresh: 500,
                  delay: 0
                }
              },
              y: { grace: 5 }
            },
            plugins: {
              legend: { display: false },
            },
            elements: {
              point: {
                borderWidth: 0,
                radius: 10,
                backgroundColor: 'rgba(0, 0, 0, 0)'
              }
            }
          }
        });
      }

      // attach listener
      let tgt = {
        id: id,
        display: data.display,
        type: data.type,
        scale: data.scale
      };

      if (data.type === 'can') {
        tgt.source = data.source;
      } else {
        let [ src, parsed ] = data.source.split(' / ');
        tgt.source = src;
        tgt.key = LOG_KEY[src].find(x => x.parsed.find(y => y === parsed)).name;
        tgt.parsed = parsed;
      }

      if (watchlist[tgt.type === 'standard' ? tgt.key : data.source.id]) {
        watchlist[tgt.type === 'standard' ? tgt.key : data.source.id].push(tgt);
      } else {
        watchlist[tgt.type === 'standard' ? tgt.key : data.source.id] = [ tgt ];
      }
    } // for (const [ did, data ] of group.data.entries())
  }
}

/************************************************************************************
 * Car ID configuratior
 ***********************************************************************************/
$('#car_id_config').click(function() {
  Swal.fire({
    html: create_html('config_car', { id: localStorage.getItem('id'), key: localStorage.getItem('key') }),
    showCancelButton: true,
    confirmButtonText: '확인',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
  }).then(result => {
    if (result.isConfirmed) {
      localStorage.setItem('id', $('#car_id').val().trim(), { expires: 365 });
      localStorage.setItem('key', $('#car_id_key').val().trim(), { expires: 365 });
      location.reload();
    }
  })
});

/************************************************************************************
 * UI configuratior
 ***********************************************************************************/
$('#ui_config').click(function() {
  Swal.fire({
    html: create_html('config_ui'),
    showCancelButton: true,
    confirmButtonText: '확인',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
    preConfirm: ui_validator,
    willOpen: function() {
      let json;

      try {
        json = JSON.parse(localStorage.getItem('ui'));
      } catch (e) {
        // do nothing
      }

      if (json) {
        for (const [ gid, group ] of json.entries()) {
          $('#ui_area').append(create_html('config_datagroup', {
            id: gid,
            default: false,
            name: group.name,
            icon: group.icon
          }));

          for (const [ did, data ] of group.data.entries()) {
            const id = `${gid}_${did}`;

            $(`#datagroup_dataarea_${gid}`).append(create_html('config_data', {
              id: id,
              default: false,
              name: data.name,
              icon: data.icon,
              display: data.display,
              scale: data.scale,
              unit: data.unit,
              type: data.type,
              source: data.source,
            }));

            datagroup[gid] = { data_count: group.data.length };
          }
          datagroup_count = json.length;
        }
      }
    }
  }).then(result => {
    if (result.isConfirmed) {
      localStorage.setItem('ui', JSON.stringify(result.value));
      location.reload();
    }
  });
});

// data validation
function ui_validator() {
  let datagroups = $('.datagroup').toArray().map(x => ({ elem: x, data: $(`#${x.id} .datagroup_data`).toArray() }) );

  let result = [];
  let target;

  // datagroup count
  if (!datagroups.length) {
    Swal.showValidationMessage('No datagroups presents!');
    return false;
  }

  /* datagroup validation */
  for (let grp of datagroups) {
    let group_id = grp.elem.id.replace('datagroup_', '');
    let group = { };

    // datagroup data element count
    target = $(`#datagroup_${group_id}`);
    if (!grp.data.length) {
      target.css('border', '2px solid red');
      Swal.showValidationMessage('Datagroup with no data presents!');
      return false;
    } else {
      target.css('border', '2px solid lightgrey');
    }

    // datagroup name
    target = $(`#datagroup_name_${group_id}`);
    if (!target.val().trim()) {
      target.css('border', '1px solid red');
      Swal.showValidationMessage('Datagroup with no name presents!');
      return false;
    } else {
      target.css('border', '1px solid #767676');
      group.name = target.val().trim();
    }

    group.icon = $(`#datagroup_iconname_${group_id}`).val().trim();

    group.data = [];

    /* group data validation */
    for (let dataitem of grp.data) {
      let data_id = dataitem.id.replace('data_', '');
      let data = { };

      // data name
      target = $(`#data_name_${data_id}`);
      target.css('border', '1px solid #767676');
      if (!target.val().trim()) {
        target.css('border', '1px solid red');
        Swal.showValidationMessage('Data with no name presents!');
        return false;
      } else {
        data.name = target.val().trim();
      }

      data.icon = $(`#data_iconname_${data_id}`).val().trim();

      // display type
      target = $(`#data_type_${data_id}`);
      target.css('border', '1px solid #767676');
      if (!target.val()) {
        target.css('border', '1px solid red');
        Swal.showValidationMessage('Data with no display type presents!');
        return false;
      } else {
        data.display = target.val();
      }

      if (data.display === 'gps') {
        group.data.push(data);
        continue;
      }

      // data type
      $(dataitem).css('border', '1px solid #dddddd');
      target = $(`input[name=data_type_${data_id}]:checked`);
      switch (target.val()) {
        case 'standard':
          data.type = 'standard';

          // data source
          target = $(`#select_data_${data_id}`);
          if (!target.val()) {
            target.css('border', '1px solid red');
            Swal.showValidationMessage('Data with no source presents!');
            return false;
          } else {
            target.css('border', '1px solid #767676');
            data.source = target.val();
          }
          break;

        case 'can':
          data.type = 'can';
          data.source = { };

          // can id
          target = $(`#can_data_id_${data_id}`);
          if (isNaN(Number(target.val().trim())) || !target.val().trim()) {
            target.css('border', '1px solid red');
            Swal.showValidationMessage('Data with no CAN ID presents!');
            return false;
          } else {
            target.css('border', '1px solid #767676');
            data.source.id = Number(target.val().trim());
          }

          // can data level
          target = $(`input[name=level_${data_id}]:checked`);
          switch (target.val()) {
            case 'byte': {
              // endianness
              target = $(`input[name=endian_${data_id}]:checked`);
              if (!(target.val() === 'big') && !(target.val() === 'little')) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with invalid endianness presents!');
                return false;
              } else {
                data.source.byte = {
                  endian: target.val()
                }
              }

              // byte range
              let start = $(`#can_start_byte_${data_id}`).val().trim();
              let end = $(`#can_end_byte_${data_id}`).val().trim();
              if (!start || isNaN(Number(start)) || Number(start) < 0 || Number(start) > 7 || Number(start) % 1 != 0) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with Invalid start byte presents!');
                return false;
              } else if (!end || isNaN(Number(end)) || Number(end) < 0 || Number(end) > 7 || Number(end) % 1 != 0) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with Invalid end byte presents!');
                return false;
              } else if (Number(start) > Number(end)) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with larger start byte then end byte presents!');
                return false;
              } else {
                data.source.byte.start = start;
                data.source.byte.end = end;
              }
              break;
            }

            case 'bit': {
              let start = $(`#can_start_bit_${data_id}`).val().trim();
              let end = $(`#can_end_bit_${data_id}`).val().trim();
              if (!start || isNaN(Number(start)) || Number(start) < 0 || Number(start) > 63 || Number(start) % 1 != 0) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with Invalid start bit presents!');
                return false;
              } else if (!end || isNaN(Number(end)) || Number(end) < 0 || Number(end) > 63 || Number(end) % 1 != 0) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with Invalid end bit presents!');
                return false;
              } else if (Number(start) > Number(end)) {
                $(dataitem).css('border', '1px solid red');
                Swal.showValidationMessage('Data with larger start bit than end bit presents!');
                return false;
              } else {
                data.source.bit = {
                  start: start,
                  end: end
                };
              }
              break;
            }

            default:
              $(dataitem).css('border', '1px solid red');
              Swal.showValidationMessage('Invalid data level(byte/bit) presents!');
              return false;
          }
          break;

        default:
          $(dataitem).css('border', '1px solid red');
          Swal.showValidationMessage('Invalid data type(standard/CAN) presents!');
          return false;
      }

      if (data.display === 'digital') {
        group.data.push(data);
        continue;
      }

      // data scale
      target = $(`#scale_${data_id}`);
      target.css('border', '1px solid #767676');
      if (isNaN(Number(target.val().trim())) || !target.val().trim()) {
        target.css('border', '1px solid red');
        Swal.showValidationMessage('Invalid data scale presents!');
        return false;
      } else {
        data.scale = Number(target.val().trim());
      }

      // data unit
      target = $(`#unit_${data_id}`);
      data.unit = target.val().trim();
      group.data.push(data);
    }
    result.push(group);
  }

  return result;
}

/************************************************************************************
 * UI configuratior events
 ***********************************************************************************/
datagroup_count = 0;
datagroup = [];
data_types = [ 'digital', 'value', 'graph', 'gps' ];

standard_records = [];
for (let key of Object.keys(LOG_KEY)) {
  if (key !== 'SYS' && key !== 'CAN') {
    for (let k of LOG_KEY[key]) {
      for (let p of k.parsed) {
        standard_records.push(`${key} / ${p}`);
      }
    }
  }
}

// add datagroup
$(document.body).on('click', '#add_data_group', e => {
  $('#ui_area').append(create_html('config_datagroup', { id: datagroup_count, default: true }));

  datagroup[datagroup_count] = { data_count: 0 };
  datagroup_count++;
});

// delete datagroup
$(document.body).on('click', '.delete_datagroup', e => {
  let target_group = e.target.id.replace('delete_datagroup_', '');

  $(`#datagroup_${target_group}`).remove();
});

// add data
$(document.body).on('click', '.add_data', e => {
  let target_group = e.target.id.replace('add_data_', '');
  let identifier = `${target_group}_${datagroup[target_group].data_count}`;

  $(`#datagroup_dataarea_${target_group}`).append(create_html('config_data', { id: identifier, default: true }));

  datagroup[target_group].data_count++;
});

// delete data
$(document.body).on('click', '.delete_data', e => {
  let target_identifier = e.target.id.replace('delete_data_', '');

  $(`#data_${target_identifier}`).remove();
});

// calculate CAN ID in hex
$(document.body).on('keyup', '.can_data_id', e => {
  let target_identifier = e.target.id.replace('can_data_id_', '');

  $(`#can_data_id_hex_${target_identifier}`).text(Number($(e.target).val()).toString(16).toUpperCase());
});

// icon previewer
$(document.body).on('keyup', '.datagroup_iconname, .data_iconname', e => {
  let type = e.target.id.includes('group') ? 'group' : '';
  let target = e.target.id.replace(`data${type}_iconname_`, '');

  $(`#data${type}_icon_${target}`).removeClass().addClass(`fa-solid fa-fw fa-${$(`#data${type}_iconname_${target}`).val()}`);
});

// display type change
$(document.body).on('change', '.data_type', e => {
  let target = e.target.id.replace('data_type_', '');

  if ($(e.target).val() === 'gps') {
    $(`#div_dataspec_${target}`).css('display', 'none');
    $(`#datainfo_${target}`).css('display', 'none');
  } else if ($(e.target).val() === 'digital') {
    $(`#div_dataspec_${target}`).css('display', 'block');
    $(`#datainfo_${target}`).css('display', 'none');
  } else {
    $(`#div_dataspec_${target}`).css('display', 'block');
    $(`#datainfo_${target}`).css('display', 'block');
  }
});

// export UI config
$(document.body).on('click', '#export_ui', e => {
  let data = ui_validator();
  if (data === false) {
    return;
  }

  let json = JSON.stringify(data, null, 2);

  const a = document.createElement('a');
  a.style.display = 'none';
  a.href = 'data:text/html;charset=utf-8,' + encodeURIComponent(json);
  a.download = 'ui_config.json';
  document.body.appendChild(a);
  a.click();
});

// import UI config
$(document.body).on('click', '#import_ui', e => {
  Swal.fire({
    title: 'ui_config.json 업로드',
    html: `<input id="file" type="file" name="file" accept=".json" style="margin-top: 1rem;">`,
    showCancelButton: true,
    confirmButtonText: '확인',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
    preConfirm: async function() {
      let file = document.getElementById('file').files[0];

      if (!file) {
        Swal.showValidationMessage('No file presents!');
        return false;
      }

      try {
        let json = JSON.parse(await readfile(file));
        return JSON.stringify(json);
      } catch (e) {
        Swal.showValidationMessage('JSON file processing error!');
        return false;
      }
    }
  }).then(result => {
    if (result.isConfirmed) {
      localStorage.setItem('ui', result.value);
      location.reload();
    }
  });
});

function readfile(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.readAsText(file);

    reader.onload = e => {
      resolve(e.target.result);
    };

    reader.onerror = reject;
  });
}

/************************************************************************************
 * HTML drawer
 ***********************************************************************************/
function create_html(type, data) {
  switch (type) {
    case 'config_car':
      return `<div style='text-align: left;'><span style='font-size: 1.7rem; font-weight: bold;'>차량 ID 설정</span></div>
      <div>
        <p>사용자 등록 시 설정한 차량 ID와 key를 입력하세요.</p>
        <p>입력한 정보는 브라우저에 저장됩니다.</p>
      </div>
      <table style='margin: auto;'>
        <tr>
          <td style='text-align: left; line-height: 1.5rem;'>차량 ID</td>
          <td>: <input id='car_id' class='data_input' value='${data.id ? data.id : ''}'></td>
        </tr>
        <tr>
          <td style='text-align: left; line-height: 1.5rem;'>key</td>
          <td>: <input id='car_id_key' type='password' class='data_input' value='${data.key ? data.key : ''}'></td>
        </tr>
      </table>
      <div style='margin-top: 1.5rem; font-size: 1rem;'>
        <p>사용자 등록 신청: <a href='mailto:mail@luftaquila.io' style='text-decoration: none; color: #0366d6'>mail@luftaquila.io</a><br>학교와 팀 이름, 사용할 차량 ID와 key를 보내주세요.</p>
      </div>`;
      break;

    case 'config_ui':
      return `<div style='text-align: left;'><span style='font-size: 1.7rem; font-weight: bold;'>UI 설정</span></div>
      <div id='ui_area'></div>
      <div style='margin-top: 1rem;'>
        <span id='add_data_group' class='btn green' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-regular fa-fw fa-plus'></i>&ensp;데이터 그룹 추가</span>
      </div>
      <div style='margin-top: 1rem;'>
        <span id='export_ui' class='btn purple' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-solid fa-fw fa-file-export'></i>&ensp;UI 설정 내보내기</span>
        <span id='import_ui' class='btn purple' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-solid fa-fw fa-file-import'></i>&ensp;UI 설정 불러오기</span>
      </div>`;
      break;

    case 'config_datagroup':
      return `<div id='datagroup_${data.id}' class='datagroup' style='text-align: left; margin-top: 2rem; border: 2px solid lightgrey; background-color: #eeeeee; border-radius: 10px; padding: 1rem;'>
        <i id='datagroup_icon_${data.id}' class='fa-solid fa-fw fa-${data.default ? 'circle-info': data.icon}'></i>&ensp;
        <input id='datagroup_name_${data.id}' value='${data.default ? '' : data.name}' placeholder='데이터그룹 이름' maxlength='20' style='font-size: 1.2rem; height: 1.8rem; width: 10rem; font-weight: bold; color: #333333; padding-left: .5rem;'>
        <span id='delete_datagroup_${data.id}' class='delete_datagroup btn red' style='height: 1.2rem; line-height: 1.2rem; transform: translate(0px, -0.25rem);'>삭제</span>
        <div style='margin-top: .5rem; margin-bottom: 1rem;'>
          <table>
            <tr>
              <td>아이콘</td>
              <td>: <input id='datagroup_iconname_${data.id}' class='datagroup_iconname' value='${data.default ? 'circle-info' : data.icon}' placeholder='아이콘 이름' maxlength='30' style='width: 6rem; height: 1.2rem; line-height: 1.2rem; padding-left: .3rem;'></td>
              <td>&ensp;<a href='https://fontawesome.com/search?o=r&m=free&s=solid' target='_blank' style='font-size: .8rem; text-decoration: underline; color: #0366d6'>검색</a></td>
            </tr>
          </table>
        </div>
        <div id='datagroup_dataarea_${data.id}' style='margin-top: .5rem; margin-bottom: .5rem;'></div>
        <div style='text-align: center;'>
        <span id='add_data_${data.id}' class='add_data btn green' style='height: 1.2rem; line-height: 1.2rem;'>
          <i class='fa-regular fa-fw fa-plus'></i>&ensp;데이터 추가</span>
        </div>
      </div>`;
      break;

    case 'config_data':
      return `<div id='data_${data.id}' class='datagroup_data' style='text-align: left; background-color: #dddddd; border: 1px solid #dddddd; border-radius: 10px; padding: .8rem; margin-bottom: 1rem;'>
      <i id='data_icon_${data.id}' class='fa-solid fa-fw fa-${data.default ? 'database' : data.icon}'></i>&ensp;
      <input id='data_name_${data.id}' value='${data.default ? '' : data.name}' placeholder='데이터 이름' maxlength='20' style='font-size: 1.1rem; width: 12rem; height: 1.5rem; padding-left: .3rem;'>
      <div style='margin-top: 1rem;'>
        <table>
          <tr>
            <td>아이콘</td>
            <td>: <input id='data_iconname_${data.id}' class='data_iconname' value='${data.default ? 'database' : data.icon}' placeholder='아이콘 이름' maxlength='30' style='width: 6rem; height: 1.2rem; line-height: 1.2rem; padding-left: .3rem;'></td>
          </tr>
          <tr>
            <td>디스플레이</td>
            <td>: <select id='data_type_${data.id}' class='data_type' style='height: 1.5rem;'><option value='' disabled ${data.default ? 'selected' : ''}>디스플레이 타입</option>${data_types.map(x => `<option value='${x}' ${(!data.default && data.display === x) ? 'selected' : ''}>${x}</option>`)}</select></td>
          </tr>
        </table>
      </div>
      <div id='div_dataspec_${data.id}' style='margin-top: 1rem; display: ${(!data.default && data.display === 'gps') ? 'none' : 'block'}'>
        <label><input type='radio' name='data_type_${data.id}' value='standard' onclick='$("#can_data_div_${data.id}").css("display", "none"); $("#standard_data_div_${data.id}").css("display", "block")' ${(data.default || data.display === 'gps' || data.type === 'standard') ? 'checked' : ''}></input>&nbsp;일반</label>&ensp;
        <label><input type='radio' name='data_type_${data.id}' value='can' onclick='$("#standard_data_div_${data.id}").css("display", "none"); $("#can_data_div_${data.id}").css("display", "block");' ${(!data.default && data.type === 'can') ? 'checked' : ''}></input>&nbsp;CAN</label>
        <div id='standard_data_div_${data.id}' style='margin-top: 1rem; margin-bottom: 1rem; display: ${(data.default || data.display === 'gps' || data.type === 'standard') ? 'block' : 'none'}'>
          <select id='select_data_${data.id}' style='width: 16rem; height: 2rem;'><option value='' disabled ${data.default || data.display === 'gps' || data.type === 'can' ? 'selected' : ''}>데이터 소스 선택</option>${standard_records.map(x => `<option value='${x}' ${(!data.default && data.source === x) ? 'selected' : ''}>${x}</option>`)}</select>
        </div>
        <div id='can_data_div_${data.id}' style='display: ${(!data.default && data.type === 'can') ? 'block' : 'none'}; margin-top: 1rem; margin-bottom: 1rem;'>
          <select id='can_favorite_${data.id}' style='width: 16rem; height: 2rem;'><option value='' disabled selected>즐겨찾기에서 선택</option>${0}</select>
          <table style='margin-top: .7rem;'>
            <tr>
              <td>CAN ID</td>
              <td>: <input id='can_data_id_${data.id}' value='${(!data.default && data.type === 'can') ? data.source.id : ''}' type='number' class='can_data_id data_input' style='width: 5rem;'>&ensp;(0x<span id='can_data_id_hex_${data.id}'>${(!data.default && data.type === 'can') ? data.source.id.toString(16).toUpperCase() : '00'}</span>)</td>
            </tr>
            <tr>
              <td>데이터</td>
              <td>
                : <label><input type='radio' name='level_${data.id}' value='byte' onclick='$("#byte_form_${data.id}").css("display", "block"); $("#bit_form_${data.id}").css("display", "none");' ${(data.default || data.display === 'gps' || (data.type === 'standard') || (data.type === 'can' && data.source.byte)) ? 'checked' : ''}></input> Byte</label>
                <label style='margin-left: .8rem;'><input type='radio' name='level_${data.id}' onclick='$("#bit_form_${data.id}").css("display", "block"); $("#byte_form_${data.id}").css("display", "none");' value='bit' ${(!data.default && data.type === 'can' && data.source.bit) ? 'checked' : ''}></input> Bit</label>
              </td>
            </tr>
          </table>
          <div id='byte_form_${data.id}' style='margin-left: 1rem; display: ${(data.default || data.display === 'gps' || (data.type === 'standard') || (data.type === 'can' && data.source.byte)) ? 'block' : 'none'}'>
            <table>
              <tr>
                <td>Endian : <label><input value='big' type='radio' name='endian_${data.id}' ${(data.default || data.display === 'gps' || data.type === 'standard' || (!data.default && data.type === 'can' && data.source.bit) || (data.type === 'can' && data.source.byte && data.source.byte.endian === 'big')) ? 'checked' : ''}></input> Big</label> <label style='margin-left: .8rem;'><input value='little' type='radio' name='endian_${data.id}' ${(!data.default && data.type === 'can' && data.source.byte && data.source.byte.endian === 'little') ? 'checked' : ''}></input> Little</label></td>
              </tr>
              <tr>
                <td>Byte : #<input id='can_start_byte_${data.id}' type='number' class='mini' value='${(!data.default && data.type === 'can' && data.source.byte) ? data.source.byte.start : 0}'> ~ #<input id='can_end_byte_${data.id}' type='number' class='mini' value='${(!data.default && data.type === 'can' && data.source.byte) ? data.source.byte.end : 0}'> <span style='font-size: .8rem;'>(#0 ~ 7)</span></td>
              </tr>
            </table>
          </div>
          <div id='bit_form_${data.id}' style='display: ${(!data.default && data.type === 'can' && data.source.bit) ? 'block' : 'none'}; margin-left: 1rem;'>
            <table style='marin-left: 1rem;'>
              <tr>
                <td>Bit</td>
                <td>: #<input id='can_start_bit_${data.id}' type='number' class='mini' value='${(!data.default && data.type === 'can' && data.source.bit) ? data.source.bit.start : 0}'> ~ #<input id='can_end_bit_${data.id}' type='number' class='mini' value='${(!data.default && data.type === 'can' && data.source.bit) ? data.source.bit.end : 0}'> <span style='font-size: .8rem;'>(#0 ~ 63)</span></td>
              </tr>
              <tr><td></td></tr>
            </table>
          </div>
          <div style='margin-top: .7rem;'><label><input id='add_to_favorite_${data.id}' type='checkbox'></input> 즐겨찾기에 추가</label></div>
        </div>
      </div>
      <table id='datainfo_${data.id}' style='display: ${data.display === 'gps' || data.display === 'digital' ? 'none' : 'block'}'>
        <tr>
          <td>데이터 단위</td>
          <td style='width: 1rem;'></td>
          <td><input id='unit_${data.id}' class='short' value='${data.default || !data.unit ? '' : data.unit}'></td>
        </tr>
        <tr>
          <td>데이터 배율</td>
          <td style='width: 1rem; text-align: right'>x</td>
          <td><input id='scale_${data.id}' type='number' class='short' value='${data.default || !data.scale ? 1 : data.scale}'></td>
        </tr>
      </table>
      <div style='text-align: center'>
        <span id='delete_data_${data.id}' class='delete_data btn red' style='height: 1.2rem; line-height: 1.2rem;'>삭제</span>
      </div>
      </div>`;
      break;

    case 'ui_datagroup':
      return `<article id='group_${data.id}'>
        <h1 style='margin-top: .3rem;'><i class='fa-solid fa-fw fa-${data.icon}'></i>&ensp;${data.name}</h1>
        <div class='content'>
          <table id='group_table_${data.id}' class='param' style='margin-top: 1.2rem;'></table>
        </div>
      </article>`;
      break;

    case 'ui_data':
      switch (data.display) {
        case 'digital':
          return `<tr>
            <th class="param-label">
              <h2><i id='icon_${data.id}' class="fa-solid fa-fw fa-1x fa-${data.icon}"></i>&ensp;${data.name}</h2>
            </th>
            <th id="data_val_${data.id}" class="param-data-digital" colspan='2'>OFF</th>
          </tr>
          <tr><td class="spacing">&ensp;</td></tr>`;
          break;

        case 'value':
          return `<tr>
            <th class="param-label">
              <h2><i id='icon_${data.id}' class="fa-solid fa-fw fa-1x fa-${data.icon}"></i>&ensp;${data.name}</h2>
            </th>
            <th id="data_val_${data.id}" class="param-data">0</th>
            <th class="param-unit">${data.unit}</th>
          </tr>
          <tr><td class="spacing">&ensp;</td></tr>`;
          break;

        case 'graph':
          return `<tr>
            <th class="param-label">
              <h2><i id='icon_${data.id}' class="fa-solid fa-fw fa-1x fa-${data.icon}"></i>&ensp;${data.name}</h2>
            </th>
            <th id="data_val_${data.id}" class="param-data">0</th>
            <th class="param-unit">${data.unit}</th>
          </tr>
          <tr>
            <td colspan="3" class="param-graph">
              <canvas id="graph_${data.id}" class="graph" width="100%" height="60vh"></canvas>
            </td>
          </tr>
          <tr><td class="spacing">&ensp;</td></tr>`;
          break;

        case 'gps':
          return `<tr><td colspan='3'><div id="map_${data.id}" style="width: 100%; height: 400px; margin-bottom: 1rem;"></div></td></tr>`;
          break;
      }
      break;
  }
}
