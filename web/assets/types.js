/* Log protocol implementation */

const LOG_LEVEL = [ 'FATAL', 'ERROR', 'WARN', 'INFO', 'DEBUG' ];
const LOG_SOURCE = [ 'SYS', 'CAN', 'DIGITAL', 'ANALOG', 'PULSE', 'ACCELEROMETER', 'GPS' ];
const LOG_KEY = {
  'SYS': [
    'SYS_SD_INIT',
    'SYS_CORE_INIT',
    'SYS_SERIAL_INIT',
    'SYS_TELEMETRY_REMOTE',
    'SYS_TELEMETRY_RTC_FIX',
    'SYS_TELEMETRY_INIT',
    'CAN_INIT',
    'DIGITAL_INIT',
    'ANALOG_INIT',
    'PULSE_INIT',
    'ACCELEROMETER_INIT',
    'GPS_INIT',
    'SYS_READY',
    'SYS_SD_FAIL',
    'CAN_ERR_CANERR',
    'CAN_ERR_RXMSGFAIL',
    'CAN_ERR_FIFOFULL'
  ],
  'CAN': [], // key is CAN message ID
  'DIGITAL': [ 'DIGITAL_DATA' ],
  'ANALOG': [ 'ANALOG_SYS', 'ANALOG_DATA' ],
  'PULSE': [ 'PULSE_DATA' ],
  'ACCELEROMETER': [ 'ACCELEROMETER_DATA' ],
  'GPS': [ 'GPS_POS', 'GPS_VEC', 'GPS_TIME' ]
};

function translate(raw) {
  try {
    let log = {
      timestamp: raw[0] + raw[1] * Math.pow(2, 8) + raw[2] * Math.pow(2, 16) + raw[3] * Math.pow(2, 24),
      datetime: null,
      level: LOG_LEVEL[raw[4]],
      source: LOG_SOURCE[raw[5]],
      key: LOG_SOURCE[raw[5]] === 'CAN' ? raw[6] : LOG_KEY[LOG_SOURCE[raw[5]]][raw[6]],
      checksum: raw[7] == ((raw[0] + raw[1] + raw[2] + raw[3] + raw[4] + raw[5] + raw[6] + raw[8] + raw[9] + raw[10] + raw[11] + raw[12] + raw[13] + raw[14] + raw[15]) % 256),
      value: raw[8] + raw[9] * Math.pow(2, 8) + raw[10] * Math.pow(2, 16) + raw[11] * Math.pow(2, 24) + raw[12] * Math.pow(2, 32) + raw[13] * Math.pow(2, 40) + raw[14] * Math.pow(2, 48) + raw[15] * Math.pow(2, 56),
      raw: raw.slice(8)
    }

    // validate checksum
    if (!log.checksum) {
      throw new Error('checksum error');
    }

    log.parsed = parse(log);

    return log;
  } catch(e) {
    return e;
  }
}

function parse(log) {
  let parsed;

  switch (source) {
    case 'SYS':
    case 'CAN':
    default:
      parsed = null;
      break;

    case 'DIGITAL': {
      switch (key) {
        case 'DIGITAL_DATA': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        default:
          parsed = null;
          break;
      }
      break;
    } // case 'DIGITAL'

    case 'ANALOG': {
      switch (key) {
        case 'ANALOG_DATA': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        case 'ANALOG_SYS': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        default:
          parsed = null;
          break;
      }
      break;
    } // case 'ANALOG'

    case 'PULSE': {
      switch (key) {
        case 'PULSE_DATA': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        default:
          parsed = null;
          break;
      }
      break;
    } // case 'PULSE'

    case 'ACCELEROMETER': {
      switch (key) {
        case 'ACCELEROMETER_DATA': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        default:
          parsed = null;
          break;
      }
      break;
    } // case 'ACCELEROMETER'

    case 'GPS': {
      switch (key) {
        case 'GPS_POS': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        case 'GPS_VEC': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        case 'GPS_TIME': {
          parsed = {
            // !!!!! TODO

          };
          break;
        }

        default:
          parsed = null;
          break;
      }
      break;
    } // case 'GPS'
  } // switch (source)
}

exports.LOG_LEVEL = LOG_LEVEL;
exports.LOG_SOURCE = LOG_SOURCE;
exports.LOG_KEY= LOG_KEY;
exports.translate = translate;
