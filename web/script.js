log = [];
filename = "";

/************************************************************************************
 * load a file
 ***********************************************************************************/
$("#file").change(async function() {
  let file = document.getElementById("file").files[0];
  if (file) {
    let reader = new FileReader();
    reader.readAsArrayBuffer(file);
    reader.onload = function (evt) {
      let raw = new Blob([evt.target.result], { type: 'application/octet-stream' });

      filename = file.name;
      process_log(raw);
    }
  }
});


/************************************************************************************
 * translate
 ***********************************************************************************/
async function process_log(raw) {
  file_date = filename.replace(".log", "").replace(" ", "-").split("-");
  file_date = new Date(file_date[0], file_date[1] - 1, file_date[2], file_date[3], file_date[4], file_date[5]);

  let buffer = await raw.arrayBuffer();
  buffer = new Uint8Array(buffer);

  const log_size = 16;
  let index = 0;
  let error = 0;
  let count = buffer.length / log_size;

  while (index < buffer.length) {
    let converted = translate(buffer.slice(index, index + log_size));

    if (converted) {
      converted.datetime = new Date(file_date.getTime() + converted.timestamp).format("yyyy-mm-dd HH:MM:ss.l");
      log.push(converted);
    } else {
      error++;
    }

    index += log_size;
  }

  // process finished
  console.log(log);

  $("#load_file_first").text(`현재 파일: ${filename}`);
  $(".btn_download").removeClass("disabled");

  draw_graph();
}


/************************************************************************************
 * download files
 ***********************************************************************************/
$("#json_download").click(function() {
  let json = JSON.stringify(log, null, 2);
  saveAs(new File([json], `${filename.replace('.log', '')}.json`, { type: 'text/json;charset=utf-8' }));
});

$("#csv_download").click(function() {
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
      while (val.length < len) val = "0" + val;
      return val;
    };
  return function (date, mask, utc) {
    var dF = dateFormat;
    if (arguments.length == 1 && Object.prototype.toString.call(date) == "[object String]" && !/\d/.test(date)) {
      mask = date;
      date = undefined;
    }
    date = date ? new Date(date) : new Date;
    if (isNaN(date)) throw SyntaxError("invalid date");
    mask = String(dF.masks[mask] || mask || dF.masks["default"]);
    if (mask.slice(0, 4) == "UTC:") {
      mask = mask.slice(4);
      utc = true;
    }
    var	_ = utc ? "getUTC" : "get",
      d = date[_ + "Date"](),
      D = date[_ + "Day"](),
      m = date[_ + "Month"](),
      y = date[_ + "FullYear"](),
      H = date[_ + "Hours"](),
      M = date[_ + "Minutes"](),
      s = date[_ + "Seconds"](),
      L = date[_ + "Milliseconds"](),
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
        t:    H < 12 ? "a"  : "p",
        tt:   H < 12 ? "am" : "pm",
        T:    H < 12 ? "A"  : "P",
        TT:   H < 12 ? "오전" : "오후",
        Z:    utc ? "UTC" : (String(date).match(timezone) || [""]).pop().replace(timezoneClip, ""),
        o:    (o > 0 ? "-" : "+") + pad(Math.floor(Math.abs(o) / 60) * 100 + Math.abs(o) % 60, 4),
        S:    ["th", "st", "nd", "rd"][d % 10 > 3 ? 0 : (d % 100 - d % 10 != 10) * d % 10]
      };
    return mask.replace(token, function ($0) {
      return $0 in flags ? flags[$0] : $0.slice(1, $0.length - 1);
    });
  };
}();
dateFormat.masks = {"default":"ddd mmm dd yyyy HH:MM:ss"};
dateFormat.i18n = {
  dayNames: [
    "일", "월", "화", "수", "목", "금", "토",
    "일요일", "월요일", "화요일", "수요일", "목요일", "금요일", "토요일"
  ],
  monthNames: [
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"
  ]
};
Date.prototype.format = function(mask, utc) { return dateFormat(this, mask, utc); };
