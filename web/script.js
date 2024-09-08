log = [];
filename = '';

let map = new kakao.maps.Map(document.getElementById('map'), {
  center: new kakao.maps.LatLng(37.2829317, 127.0435822)
});
Chart.register(window['chartjs-plugin-autocolors']);

/************************************************************************************
 * load a file
 ***********************************************************************************/
$('#file').change(async function() {
  let file = document.getElementById('file').files[0];
  if (file) {
    let reader = new FileReader();
    reader.readAsArrayBuffer(file);
    reader.onload = function (evt) {
      $('#load_file_first').text(`파일 변환 중...`);

      let raw = new Blob([evt.target.result], { type: 'application/octet-stream' });
      filename = file.name;
      process_log(raw);
      $('#file').closest('div.content').remove();
    }
  }
});


/************************************************************************************
 * translate
 ***********************************************************************************/
async function process_log(raw) {
  file_date = filename.replace('.log', '').replace(' ', '-').split('-');
  file_date = new Date(file_date[0], file_date[1] - 1, file_date[2], file_date[3], file_date[4], file_date[5]);

  let buffer = await raw.arrayBuffer();
  buffer = new Uint8Array(buffer);

  const log_size = 16;
  let index = 0;
  let error = 0;
  let count = buffer.length / log_size;

  while (index < buffer.length) {
    let converted = translate(buffer.slice(index, index + log_size));

    if (!(converted instanceof Error)) {
      converted.datetime = new Date(file_date.getTime() + converted.timestamp).format('yyyy-mm-dd HH:MM:ss.l');
      log.push(converted);
    } else {
      error++;
    }

    index += log_size;
  }

  // process finished
  console.log(log);

  $('#load_file_first').text(`현재 파일: ${filename}`);
  $('.btn_download, #add_graph').removeClass('disabled');

  $("#data-count").text(count);
  $("#error-count").text(error);
  $("#converted-count").text(count - error);
  $("#duration").text(((log[log.length - 1].timestamp - log[0].timestamp) / (1000 * 60)).toFixed(0));

  init_viewer(log);
}


/************************************************************************************
 * initialize graph and map data
 ***********************************************************************************/
function init_viewer(log) {
  let keylist = [];
  let gps = [];

  for (let data of log) {
    if (data.source == 'GPS' && data.key == 'GPS_POS') {
      gps.push(new kakao.maps.LatLng(data.parsed.lat, data.parsed.lon));
    } else if (data.parsed !== null && !keylist.find(x => x.key == data.key)) {
      keylist.push(data);
    }
  }

  if (gps.length) {
    new kakao.maps.Polyline({
      path: gps,
      strokeWeight: 5,
      strokeColor: '#0080FF',
      strokeOpacity: 0.7,
      strokeStyle: 'solid'
    }).setMap(map);

    map.panTo(gps[0]);
  }

  param = [];
  for (let key of keylist) {
    for (let p of Object.keys(key.parsed)) {
      param.push({
        source: key.source,
        key: key.key,
        param: p
      });
    }
  }
}


/************************************************************************************
 * graph manipulation functions
 ***********************************************************************************/
graph_count = 0;

$('#add_graph').click(async function() {
  $('#graphs').append(`<div id='graph_${graph_count}'>
  <div>
    <span id='add_graph_data_${graph_count}' class='add_graph_data btn green' style='height: 1.5rem; line-height: 1.5rem;'>
      <i class='fa-solid fa-fw fa-magnifying-glass-plus'></i>&ensp;데이터 추가</span>
    <span id='delete_graph_${graph_count}' class='delete_graph btn red' style='height: 1.5rem; line-height: 1.5rem;'>
      <i class='fa-solid fa-fw fa-x'></i>&ensp;그래프 삭제</span>
  </div>
  <canvas id='graph_canvas_${graph_count}' class='graph canvas_disabled' width='100%' height='55vh'></canvas>
  <hr style='margin-top: 1.5rem; margin-bottom: 1rem;'>
</div>`);

  const canvas = $(`#graph_canvas_${graph_count}`);

  let chart = new Chart(canvas, {
    type: 'line',
    data: {
      datasets: [],
    },
    options: {
      responsive: true,
      interaction: {
        mode: 'nearest',
        axis: 'x',
        intersect: false,
      },
      elements: {
        point: {
          hitRadius: 10,
        }
      },
      scales: {
        x: {
          type: 'time',
          time: {
            unit: 'second',
            displayFormats: {
              second: 'H:mm:ss'
            }
          },
        },
      },
      plugins: {
        zoom: {
          zoom: {
            wheel: {
              enabled: true,
            },
            pinch: {
              enabled: true,
            },
            mode: 'x',
          }
        },
        tooltip: {
          position: 'bottom',
        },
        crosshair: {
          line: {
            color: 'black',
            width: 1,
          },
          sync: {
            enabled: true,
            group: 1,
            suppressTooltips: false,
          },
          zoom: {
            enabled: true,
            zoomButtonText: '줌 리셋',
            zoomButtonClass: 'reset-zoom btn purple',
          },
          callbacks: {
            beforeZoom: () => function(start, end) {
              return true;
            },
            afterZoom: () => function(start, end) {
            }
          }
        },
      }
    },
    elements: {
      point: {
        borderWidth: 0,
        radius: 2,
        backgroundColor: 'rgba(0, 0, 0, 0)'
      }
    }
  });
  canvas.data('graph', chart);

  graph_count++;
});

$(document.body).on('click', '.delete_graph', e => {
  $(`#graph_${e.target.id.replace('delete_graph_', '')}`).remove();
});

$(document.body).on('click', '.add_graph_data', async e => {
  let target_graph = e.target.id.replace('add_graph_data_', '');

  let html = `<div style='text-align: left'>
  <label>
    <span style='font-weight: bold; font-size: 1.25rem;'>
      <input type='radio' name='data_type' value='standard' onclick='$("#can_data_div").addClass("disabled"); $("#standard_data_div").removeClass("disabled")' checked></input>&ensp;일반 데이터 추가
    </span>
  </label>
  <div id='standard_data_div' style='margin-top: 1rem; margin-bottom: 2rem; margin-left: 2rem;'>
    <select id='select_data' style='width: 18rem; height: 2rem;'>
      <option value='' disabled selected>그래프에 추가할 데이터를 선택하세요.</option>${param.map(x => `<option value='${x.source}/${x.key}/${x.param}'>${x.source} / ${x.param}</option>`)}
    </select>
  </div>
  <label>
    <span style='font-weight: bold; font-size: 1.25rem;'>
      <input type='radio' name='data_type' value='can' onclick='$("#standard_data_div").addClass("disabled"); $("#can_data_div").removeClass("disabled");'></input>&ensp;CAN 데이터 추가
    </span>
  </label>
  <div id='can_data_div' class='disabled' style='margin-top: 1rem; margin-bottom: 2rem; margin-left: 2rem;'>
    <!--
    <select id='can_favorite' style='width: 18rem; height: 2rem;'><option value='' disabled selected>즐겨찾기에서 선택</option>${0}</select>
    -->
    <table style='margin-top: .7rem;'>
      <tr>
        <td>레이블</td>
        <td>: <input id='data_label' class='data_input'></td>
      </tr>
      <tr>
        <td>CAN ID</td>
        <td>: <input id='can_data_id' type='number' class='data_input'>&ensp;(0x<span id='can_data_id_hex'>00</span>)</td>
      </tr>
      <tr>
        <td>데이터</td>
        <td>: <label><input type='radio' name='level' value='byte' onclick='$("#byte_form").css("display", "block"); $("#bit_form").css("display", "none");' checked></input> Byte</label> <label style='margin-left: .8rem;'><input type='radio' name='level' onclick='$("#bit_form").css("display", "block"); $("#byte_form").css("display", "none");' value='bit'></input> Bit</label></td>
      </tr>
    </table>
    <div id='byte_form' style='margin-left: 2rem;'>
      <table style='marin-left: 1rem;'>
        <tr>
          <td>Endian</td>
          <td>: <label><input value='big' type='radio' name='endian' checked></input> Big</label> <label style='margin-left: .8rem;'><input value='little' type='radio' name='endian'></input> Little</label></td>
        </tr>
        <tr>
          <td>Byte</td>
          <td>: #<input id='can_start_byte' type='number' class='mini' value='0'> ~ #<input id='can_end_byte' type='number' class='mini' value='0'> <span style='font-size: .8rem;'>(#0 ~ #7)</span></td>
        </tr>
      </table>
    </div>
    <div id='bit_form' style='display: none; margin-left: 2rem;'>
      <table style='marin-left: 1rem;'>
        <tr>
          <td>Bit</td>
          <td>: #<input id='can_start_bit' type='number' class='mini' value='0'> ~ #<input id='can_end_bit' type='number' class='mini' value='0'> <span style='font-size: .8rem;'>(#0 ~ #63)</span></td>
        </tr>
        <tr><td></td></tr>
      </table>
    </div>
    <!--
    <div style='margin-top: .7rem;'><label><input id='add_to_favorite' type='checkbox'></input> 즐겨찾기에 추가</label></div>
    -->
  </div>
  <div><span style='font-weight: bold;'>데이터 배율</span>&ensp;&ensp;x <input id='scale' type='number' class='short' value=1></div>
</div>`;

  Swal.fire({
    html: html,
    showCancelButton: true,
    confirmButtonText: '데이터 추가',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
    preConfirm: function() {
      // data validation
      switch ($("input[name=data_type]:checked").val()) {
        case 'standard':
          if (!$('#select_data').val()) {
            Swal.showValidationMessage('No selected data!');
            return false;
          }
          break;

        case 'can':
          if (!$('#data_label').val().trim()) {
            Swal.showValidationMessage('No data label!');
            return false;
          } else if (!$('#can_data_id').val().trim() || isNaN(Number($('#can_data_id').val().trim()))) {
            Swal.showValidationMessage('Invalid CAN ID!');
            return false;
          }

          switch ($("input[name=level]:checked").val()) {
            case 'byte':
              if (!$("input[name=endian]:checked").val()) {
                Swal.showValidationMessage('No endian specified!');
                return false;
              } else {
                let start = $("#can_start_byte").val().trim();
                let end = $("#can_end_byte").val().trim();
                if (!start || isNaN(Number(start)) || Number(start) < 0 || Number(start) > 7 || Number(start) % 1 != 0) {
                  Swal.showValidationMessage('Invalid start byte!');
                  return false;
                } else if (!end || isNaN(Number(end)) || Number(end) < 0 || Number(end) > 7 || Number(end) % 1 != 0) {
                  Swal.showValidationMessage('Invalid end byte!');
                  return false;
                } else if (Number(start) > Number(end)) {
                  Swal.showValidationMessage('Start byte number is larger than end byte number!');
                  return false;
                }
              }
              break;

            case 'bit': {
              let start = $("#can_start_bit").val().trim();
              let end = $("#can_end_bit").val().trim();
              if (!start || isNaN(Number(start)) || Number(start) < 0 || Number(start) > 63 || Number(start) % 1 != 0) {
                Swal.showValidationMessage('Invalid start bit!');
                return false;
              } else if (!end || isNaN(Number(end)) || Number(end) < 0 || Number(end) > 63 || Number(end) % 1 != 0) {
                Swal.showValidationMessage('Invalid end bit!');
                return false;
              } else if (Number(start) > Number(end)) {
                Swal.showValidationMessage('Start bit number is larger than end bit number!');
                return false;
              }
              break;
            }

            default:
              Swal.showValidationMessage('Invalid data byte/bit configuration!');
              return false;
          }
          break;

        default:
          Swal.showValidationMessage('Invalid data type(standard/CAN)!');
          return false;
          break;
      }

      let scale = $('#scale').val().trim();
      if (!scale || isNaN(Number(scale))) {
        Swal.showValidationMessage('Invalid scale value!');
        return false;
      }

      return scale;
    }
  }).then(result => {
    if (result.isConfirmed) {
      let data = {
        target: target_graph,
        scale: Number(result.value),
      };

      switch($("input[name=data_type]:checked").val()) {
        case 'standard':
          data.type = 'standard';
          data.data = $('#select_data').val();
          break;
        case 'can':
          switch ($("input[name=level]:checked").val()) {
            case 'byte':
              data.type = 'can';
              data.data = {
                label: $('#data_label').val(),
                id: Number($('#can_data_id').val()),
                type: 'byte',
                endian: $("input[name=endian]:checked").val(),
                start: Number($("#can_start_byte").val()),
                end: Number($("#can_end_byte").val())
              };
              break;
            case 'bit':
              data.type = 'can';
              data.data = {
                label: $('#data_label').val(),
                id: Number($('#can_data_id').val()),
                type: 'bit',
                start: Number($("#can_start_bit").val()),
                end: Number($("#can_end_bit").val())
              };
              break;
          }
          break;
      }
      add_graph_data(data);
    }
  });
});


function add_graph_data(data) {
  const chart = $(`#graph_canvas_${data.target}`).data('graph');

  switch (data.type) {
    case 'standard': {
      const [source, key, param] = data.data.split('/');
      const arr = log.filter(x => x.key == key);

      let dataset = arr.map(k => { return { x: new Date(k.datetime).getTime(), y: k.parsed[param] * data.scale } });

      chart.data.datasets.push({
        label: data.data.replace(/\/.*\//, ' / '),
        data: dataset,
      });
      break;
    }

    case 'can': {
      const arr = log.filter(x => (x.source === 'CAN' && x.key === data.data.id ));

      let dataset = arr.map(k => { return { x: new Date(k.datetime).getTime(), y: parse_CAN({ type: data.data.type, info: data.data }, k.raw) * data.scale  } });

      chart.data.datasets.push({
        label: data.data.label,
        data: dataset,
      });
      break;
    }

    default:
      return;
  }

  chart.update();

  $(`#graph_canvas_${data.target}`).removeClass('canvas_disabled');
}


$(document.body).on('keyup', '#can_data_id', e => {
  $('#can_data_id_hex').text(Number($('#can_data_id').val()).toString(16).toUpperCase());
});


/************************************************************************************
 * download files
 ***********************************************************************************/
$('#json_download').click(function() {
  let json = JSON.stringify(log, null, 2);
  saveAs(new File([json], `${filename.replace('.log', '')}.json`, { type: 'text/json;charset=utf-8' }));
});

$('#csv_download').click(function() {
  let csv = array_to_csv(log);
  saveAs(new File([csv], `${filename.replace('.log', '')}.csv`, { type: 'text/csv;charset=utf-8' }));
});


/************************************************************************************
 * convert array to csv
 ***********************************************************************************/
function array_to_csv(array) {
  let mp = new Map();

  function setValue(a, path, val) {
    if (Object(val) !== val) {
      const path_str = path.join('/');
      let i = (mp.has(path_str) ? mp : mp.set(path_str, mp.size)).get(path_str);
      a[i] = val;
    } else {
      for (let key in val) {
        setValue(a, key == '0' ? path : path.concat(key), val[key]);
      }
    }
    return a;
  }

  let result = array.map( obj => setValue([], [], obj) );

  array = [[...mp.keys()], ...result];

  return array.map( row => row.map ( val => isNaN(val) ? JSON.stringify(val) : +val ).join(',') ).join('\n');
}


/************************************************************************************
 * new Date().format()
 ***********************************************************************************/
var dateFormat = function () {
  var	token = /d{1,4}|m{1,4}|yy(?:yy)?|([HhMsTt])\1?|[LloSZ]|"[^"]*"|'[^']*'/g,
    timezone = /\b(?:[PMCEA][SDP]T|(?:Pacific|Mountain|Central|Eastern|Atlantic) (?:Standard|Daylight|Prevailing) Time|(?:GMT|UTC)(?:[-+]\d{4})?)\b/g,
    timezoneClip = /[^-+\dA-Z]/g,
    pad = function (val, len) {
      val = String(val);
      len = len || 2;
      while (val.length < len) val = '0' + val;
      return val;
    };
  return function (date, mask, utc) {
    var dF = dateFormat;
    if (arguments.length == 1 && Object.prototype.toString.call(date) == '[object String]' && !/\d/.test(date)) {
      mask = date;
      date = undefined;
    }
    date = date ? new Date(date) : new Date;
    if (isNaN(date)) throw SyntaxError('invalid date');
    mask = String(dF.masks[mask] || mask || dF.masks['default']);
    if (mask.slice(0, 4) == 'UTC:') {
      mask = mask.slice(4);
      utc = true;
    }
    var	_ = utc ? 'getUTC' : 'get',
      d = date[_ + 'Date'](),
      D = date[_ + 'Day'](),
      m = date[_ + 'Month'](),
      y = date[_ + 'FullYear'](),
      H = date[_ + 'Hours'](),
      M = date[_ + 'Minutes'](),
      s = date[_ + 'Seconds'](),
      L = date[_ + 'Milliseconds'](),
      o = utc ? 0 : date.getTimezoneOffset(),
      flags = {
        d:    d,
        dd:   pad(d),
        ddd:  dF.i18n.dayNames[D],
        dddd: dF.i18n.dayNames[D + 7],
        m:    m + 1,
        mm:   pad(m + 1),
        mmm:  dF.i18n.monthNames[m],
        mmmm: dF.i18n.monthNames[m + 12],
        yy:   String(y).slice(2),
        yyyy: y,
        h:    H % 12 || 12,
        hh:   pad(H % 12 || 12),
        H:    H,
        HH:   pad(H),
        M:    M,
        MM:   pad(M),
        s:    s,
        ss:   pad(s),
        l:    pad(L, 3),
        L:    pad(L > 99 ? Math.round(L / 10) : L),
        t:    H < 12 ? 'a'  : 'p',
        tt:   H < 12 ? 'am' : 'pm',
        T:    H < 12 ? 'A'  : 'P',
        TT:   H < 12 ? '오전' : '오후',
        Z:    utc ? 'UTC' : (String(date).match(timezone) || ['']).pop().replace(timezoneClip, ''),
        o:    (o > 0 ? '-' : '+') + pad(Math.floor(Math.abs(o) / 60) * 100 + Math.abs(o) % 60, 4),
        S:    ['th', 'st', 'nd', 'rd'][d % 10 > 3 ? 0 : (d % 100 - d % 10 != 10) * d % 10]
      };
    return mask.replace(token, function ($0) {
      return $0 in flags ? flags[$0] : $0.slice(1, $0.length - 1);
    });
  };
}();
dateFormat.masks = {'default':'ddd mmm dd yyyy HH:MM:ss'};
dateFormat.i18n = {
  dayNames: [
    '일', '월', '화', '수', '목', '금', '토',
    '일요일', '월요일', '화요일', '수요일', '목요일', '금요일', '토요일'
  ],
  monthNames: [
    'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec',
    'January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'
  ]
};
Date.prototype.format = function(mask, utc) { return dateFormat(this, mask, utc); };


/************************************************************************************
 * chart.js functions
 ***********************************************************************************/
Chart.Tooltip.positioners.bottom = function(items, coord) {
  const pos = Chart.Tooltip.positioners.average(items);

  // Happens when nothing is found
  if (pos === false) {
    return false;
  }

  const chart = this.chart;

  return {
    x: coord.x,
    y: chart.chartArea.top,
    xAlign: 'center',
    yAlign: 'top',
  };
};
