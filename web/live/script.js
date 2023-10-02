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

socket = io.connect("/", {
  query: {
    client: true,
    channel: Cookies.get('id'),
    key: Cookies.get('key')
  }
});

socket.on('connect', () => {
  $("#server i").css("color", "green");
});

socket.on('connect_error', () => {
  $("#server i").css("color", "red");
});

socket.on('disconnect', () => {
  $("#server i").css("color", "red");

  if (!Cookies.get('id') && !Cookies.get('key')) {
    Swal.fire({
      icon: 'info',
      title: '차량 ID 정보 없음',
      html: '<div style="line-height: 2.5rem;">차량 ID가 설정되지 않았습니다.<br>먼저 차량 ID와 key를 설정해 주세요.</div>',
      confirmButtonText: '확인',
      customClass: {
        confirmButton: 'btn green',
      }
    });
  } else {
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
});

/************************************************************************************
 * Car ID configuratior
 ***********************************************************************************/
$('#car_id_config').click(function() {
  let html = `<div style='text-align: left;'><span style='font-size: 1.7rem; font-weight: bold;'>차량 ID 설정</span></div>
  <div>
    <p>사용자 등록 시 설정한 차량 ID와 key를 입력하세요.</p>
    <p>입력한 정보는 브라우저 쿠키에 저장됩니다.</p>
  </div>
  <table style='margin: auto;'>
    <tr>
      <td style='text-align: left; line-height: 1.5rem;'>차량 ID</td>
      <td>: <input id='car_id' class='data_input' value='${Cookies.get('id') ? Cookies.get('id') : ''}'></td>
    </tr>
    <tr>
      <td style='text-align: left; line-height: 1.5rem;'>key</td>
      <td>: <input id='car_id_key' class='data_input' value='${Cookies.get('key') ? Cookies.get('key') : ''}'></td>
    </tr>
  </table>`;

  Swal.fire({
    html: html,
    showCancelButton: true,
    confirmButtonText: '확인',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
  }).then(result => {
    if (result.isConfirmed) {
      Cookies.set('id', $('#car_id').val().trim(), { expires: 365 });
      Cookies.set('key', $('#car_id_key').val().trim(), { expires: 365 });
      location.reload();
    }
  })
});

/************************************************************************************
 * UI configuratior
 ***********************************************************************************/
$('#ui_config').click(function() {
  let html = `<div style='text-align: left;'><span style='font-size: 1.7rem; font-weight: bold;'>UI 설정</span></div>
  <div id='ui_area'></div>
  <div style='margin-top: 1rem;'>
    <span id='add_data_group' class='btn green' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-regular fa-fw fa-plus'></i>&ensp;데이터 그룹 추가</span>
  </div>
  <div style='margin-top: 1rem;'>
    <span id='export_ui' class='btn purple' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-solid fa-fw fa-file-export'></i>&ensp;UI 설정 내보내기</span><br>
    <span id='import_ui' class='btn purple' style='height: 1.5rem; line-height: 1.5rem;'><i class='fa-solid fa-fw fa-file-import'></i>&ensp;UI 설정 불러오기</span>
  </div>`;

  Swal.fire({
    html: html,
    showCancelButton: true,
    confirmButtonText: '확인',
    cancelButtonText: '취소',
    customClass: {
      confirmButton: 'btn green',
      cancelButton: 'btn red'
    },
    preConfirm: function() {
      // data validation TODO

    }
  }).then(result => {
    if (result.isConfirmed) {

    }
  });
});

/************************************************************************************
 * UI configuratior events
 ***********************************************************************************/
datagroup_count = 0;
datagroup = [];
data_types = [ 'digital', 'state', 'value', 'graph', 'gps' ];

// add datagroup
$(document.body).on('click', '#add_data_group', e => {
  let datagroup_html = `<div id='datagroup_${datagroup_count}'style='text-align: left; margin-top: 2rem; border: 2px solid lightgrey; background-color: #eeeeee; border-radius: 10px; padding: 1rem;'>
    <i id='datagroup_icon_${datagroup_count}' class='fa-solid fa-fw fa-info'></i>&ensp;
    <input id='datagroup_name_${datagroup_count}' placeholder='데이터그룹 이름' maxlength='20' style='font-size: 1.2rem; height: 1.8rem; width: 10rem; font-weight: bold; color: #333333; padding-left: .5rem;'>
    <span id='delete_datagroup_${datagroup_count}' class='delete_datagroup btn red' style='height: 1.2rem; line-height: 1.2rem; transform: translate(0px, -0.25rem);'>삭제</span>
    <div style='margin-top: .5rem; margin-bottom: 1rem;'>
      <table>
        <tr>
          <td>아이콘</td>
          <td>: <input id='datagroup_iconname_${datagroup_count}' class='datagroup_iconname' value='info' placeholder='아이콘 이름' maxlength='30' style='width: 6rem; height: 1.2rem; line-height: 1.2rem; padding-left: .3rem;'></td>
          <td>&ensp;<a href='https://fontawesome.com/search?o=r&m=free&s=solid' target='_blank' style='font-size: .8rem; text-decoration: underline; color: #0366d6'>검색</a></td>
        </tr>
      </table>
    </div>
    <div id='datagroup_dataarea_${datagroup_count}' style='margin-top: .5rem; margin-bottom: .5rem;'></div>
    <div style='text-align: center;'>
    <span id='add_data_${datagroup_count}' class='add_data btn green' style='height: 1.2rem; line-height: 1.2rem;'>
      <i class='fa-regular fa-fw fa-plus'></i>&ensp;데이터 추가</span>
    </div>
  </div>`;
  $('#ui_area').append(datagroup_html);

  datagroup[datagroup_count] = {
    data_count: 0,
    data_list: [],
  };

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

  let standard_records = [];
  for (let key of Object.keys(LOG_KEY)) {
    if (key !== 'SYS' && key !== 'CAN') {
      for (let k of LOG_KEY[key]) {
        for (let p of k.parsed) {
          standard_records.push(`${key} / ${p}`);
        }
      }
    }
  }

  let data_html = `<div id='data_${identifier}' style='text-align: left; background-color: #dddddd; border-radius: 10px; padding: .8rem; margin-bottom: 1rem;'>
  <i id='data_icon_${identifier}' class='fa-solid fa-fw fa-database'></i>&ensp;
  <input id='data_name_${identifier}' placeholder='데이터 이름' maxlength='20' style='font-size: 1.1rem; width: 12rem; height: 1.5rem; padding-left: .3rem;'>
  <div style='margin-top: 1rem;'>
    <table>
      <tr>
        <td>아이콘</td>
        <td>: <input id='data_iconname_${identifier}' class='data_iconname' value='database' placeholder='아이콘 이름' maxlength='30' style='width: 6rem; height: 1.2rem; line-height: 1.2rem; padding-left: .3rem;'></td>
        <td>&ensp;<a href='https://fontawesome.com/search?o=r&m=free&s=solid' target='_blank' style='font-size: .8rem; text-decoration: underline; color: #0366d6'>검색</a></td>
      </tr>
      <tr>
        <td>디스플레이</td>
        <td>: <select id='data_type_${identifier}' style='height: 1.5rem;'><option value='' disabled selected>디스플레이 타입</option>${data_types.map(x => `<option value='${x}'>${x}</option>`)}</select></td>
      </tr>
    </table>
  </div>
  <div style='margin-top: 1rem;'>
    <label><input type='radio' name='data_type_${identifier}' value='standard' onclick='$("#can_data_div_${identifier}").css("display", "none"); $("#standard_data_div_${identifier}").css("display", "block")' checked></input>&nbsp;일반</label>&ensp;
    <label><input type='radio' name='data_type_${identifier}' value='can' onclick='$("#standard_data_div_${identifier}").css("display", "none"); $("#can_data_div_${identifier}").css("display", "block");'></input>&nbsp;CAN</label>
    <div id='standard_data_div_${identifier}' style='margin-top: 1rem; margin-bottom: 1rem;'>
      <select id='select_data_${identifier}' style='width: 16rem; height: 2rem;'><option value='' disabled selected>데이터 소스 선택</option>${standard_records.map(x => `<option value='${x}'>${x}</option>`)}</select>
    </div>
    <div id='can_data_div_${identifier}' style='display: none; margin-top: 1rem; margin-bottom: 1rem;'>
      <select id='can_favorite_${identifier}' style='width: 16rem; height: 2rem;'><option value='' disabled selected>즐겨찾기에서 선택</option>${0}</select>
      <table style='margin-top: .7rem;'>
        <tr>
          <td>CAN ID</td>
          <td>: <input id='can_data_id_${identifier}' type='number' class='can_data_id data_input' style='width: 5rem;'>&ensp;(0x<span id='can_data_id_hex_${identifier}'>00</span>)</td>
        </tr>
        <tr>
          <td>데이터</td>
          <td>
            : <label><input type='radio' name='level_${identifier}' value='byte' onclick='$("#byte_form_${identifier}").css("display", "block"); $("#bit_form_${identifier}").css("display", "none");' checked></input> Byte</label>
            <label style='margin-left: .8rem;'><input type='radio' name='level_${identifier}' onclick='$("#bit_form_${identifier}").css("display", "block"); $("#byte_form_${identifier}").css("display", "none");' value='bit'></input> Bit</label>
          </td>
        </tr>
      </table>
      <div id='byte_form_${identifier}' style='margin-left: 1rem;'>
        <table>
          <tr>
            <td>Endian : <label><input value='big' type='radio' name='endian_${identifier}' checked></input> Big</label> <label style='margin-left: .8rem;'><input value='little' type='radio' name='endian_${identifier}'></input> Little</label></td>
          </tr>
          <tr>
            <td>Byte : #<input id='can_start_byte_${identifier}' type='number' class='mini' value='0'> ~ #<input id='can_end_byte_${identifier}' type='number' class='mini' value='0'> <span style='font-size: .8rem;'>(#0 ~ 7)</span></td>
          </tr>
        </table>
      </div>
      <div id='bit_form_${identifier}' style='display: none; margin-left: 1rem;'>
        <table style='marin-left: 1rem;'>
          <tr>
            <td>Bit</td>
            <td>: #<input id='can_start_bit_${identifier}' type='number' class='mini' value='0'> ~ #<input id='can_end_bit_${identifier}' type='number' class='mini' value='0'> <span style='font-size: .8rem;'>(#0 ~ 63)</span></td>
          </tr>
          <tr><td></td></tr>
        </table>
      </div>
      <div style='margin-top: .7rem;'><label><input id='add_to_favorite_${identifier}' type='checkbox'></input> 즐겨찾기에 추가</label></div>
    </div>
  </div>
  <div> <span>데이터 배율</span>&ensp;&ensp;x <input id='mag_${identifier}' type='number' class='short' value=1> </div>
  <div style='text-align: center'>
    <span id='delete_data_${identifier}' class='delete_data btn red' style='height: 1.2rem; line-height: 1.2rem;'>삭제</span>
  </div>
  </div>`;

  $(`#datagroup_dataarea_${target_group}`).append(data_html);

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


// datagroup icon previewer
$(document.body).on('keyup', '.datagroup_iconname', e => {
  let target_group = e.target.id.replace('datagroup_iconname_', '');

  $(`#datagroup_icon_${target_group}`).removeClass().addClass(`fa-solid fa-fw fa-${$(`#datagroup_iconname_${target_group}`).val()}`);
});


// data icon previewer
$(document.body).on('keyup', '.data_iconname', e => {
  let target_identifier = e.target.id.replace('data_iconname_', '');

  $(`#data_icon_${target_identifier}`).removeClass().addClass(`fa-solid fa-fw fa-${$(`#data_iconname_${target_identifier}`).val()}`);
});
