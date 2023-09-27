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

  let source = log.source;
  let key = log.key;
  let raw = log.raw;

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
            DIN0: raw[0],
            DIN1: raw[1],
            DIN2: raw[2],
            DIN3: raw[3],
            DIN4: raw[4],
            DIN5: raw[5],
            DIN6: raw[6],
            DIN7: raw[7],
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
      const resolution = 12; // ADC resolution in bits
      const max = (1 << resolution) - 1;

      switch (key) {
        case 'ANALOG_DATA': {
          parsed = {
            AIN0: (raw[0] + raw[1] * Math.pow(2, 8)) / max,
            AIN1: (raw[2] + raw[3] * Math.pow(2, 8)) / max,
            AIN2: (raw[4] + raw[5] * Math.pow(2, 8)) / max,
            AIN3: (raw[6] + raw[7] * Math.pow(2, 8)) / max,
          };
          break;
        }

        case 'ANALOG_SYS': {
          parsed = {
            CPU_TEMP: (raw[0] + raw[1] * Math.pow(2, 8)) / max,
            INPUT_VOLTAGE: (raw[2] + raw[3] * Math.pow(2, 8)) / max,
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
            PIN0: (raw[0] + raw[1] * Math.pow(2, 8)),
            PIN1: (raw[2] + raw[3] * Math.pow(2, 8)),
            PIN2: (raw[4] + raw[5] * Math.pow(2, 8)),
            PIN3: (raw[6] + raw[7] * Math.pow(2, 8)),
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
            x: 4 / 512 * signed(raw[0] + raw[1] * Math.pow(2, 8), 16),
            y: 4 / 512 * signed(raw[2] + raw[3] * Math.pow(2, 8), 16),
            z: 4 / 512 * signed(raw[4] + raw[5] * Math.pow(2, 8), 16),
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
          const raw_lat = (raw[0] + raw[1] * Math.pow(2, 8) + raw[2] * Math.pow(2, 16) + raw[3] * Math.pow(2, 24)) * 0.0000001;
          const raw_lon = (raw[4] + raw[5] * Math.pow(2, 8) + raw[6] * Math.pow(2, 16) + raw[7] * Math.pow(2, 24)) * 0.0000001;

          parsed = {
            lat: Math.floor(raw_lat) + (((raw_lat % 1) * 100).toFixed(5) / 60),
            lon: Math.floor(raw_lon) + (((raw_lon % 1) * 100).toFixed(5) / 60),
          };
          break;
        }

        case 'GPS_VEC': {
          parsed = {
            speed: (raw[0] + raw[1] * Math.pow(2, 8) + raw[2] * Math.pow(2, 16) + raw[3] * Math.pow(2, 24)) * 0.01,
            course: (raw[4] + raw[5] * Math.pow(2, 8) + raw[6] * Math.pow(2, 16) + raw[7] * Math.pow(2, 24))
          };
          break;
        }

        case 'GPS_TIME': {
          parsed = {
            utc_date: (raw[0] + raw[1] * Math.pow(2, 8) + raw[2] * Math.pow(2, 16) + raw[3] * Math.pow(2, 24)),
            utc_time: (raw[4] + raw[5] * Math.pow(2, 8) + raw[6] * Math.pow(2, 16) + raw[7] * Math.pow(2, 24))
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

function signed(value, bit) {
  return value > Math.pow(2, bit - 1) - 1 ? value - Math.pow(2, bit) : value;
}

exports.LOG_LEVEL = LOG_LEVEL;
exports.LOG_SOURCE = LOG_SOURCE;
exports.LOG_KEY= LOG_KEY;
exports.translate = translate;
