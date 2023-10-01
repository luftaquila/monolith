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

socket = io.connect("/", { query: {
  client: true,
  channel: Cookies.get('id'),
  key: Cookies.get('key')
} });

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
  <span id='add_data_group' class='btn green' style='height: 1.5rem; line-height: 1.5rem;'>
    <i class='fa-solid fa-fw fa-object-group'></i>&ensp;데이터 그룹 추가</span>
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
    preconfirm: function() {

    }
  }).then(result => {
    if (result.isConfirmed) {

    }
  });
});

/************************************************************************************
 * UI configuratior events
 ***********************************************************************************/
$(document.body).on('click', 'add_data_group', e => {
  let datagroup_html = ``;
  $('#ui_area').append(datagroup_html);

});

let datagroup = [];

class Viewer {
  constructor(type, label, unit, icon) {
    this.type = type;
    this.label = label;
    this.unit = unit;
    this.icon = icon;
    this.data = { };

    switch (this.type) {
      case 'digital':
        this.html = `<tr>`;
        break;

      case 'state':
        break;

      case 'value':
        break;

      case 'graph':
        break;

      case 'gps':
        break;
    }
  }

  toString() {
    return JSON.stringify({
      type: this.type,
      label: this.label,
      unit: this.unit,
      icon: this.icon,
      html: this.html,
    });
  }
}
