import fs from 'fs'
import { Server } from 'socket.io'
import dateFormat from 'dateformat'
import winston from 'winston';
import winstonDaily from 'winston-daily-rotate-file';

import { LOG_LEVEL, LOG_SOURCE, LOG_KEY, translate } from '../web/assets/types.js';

/*****************************************************************************
 * server configurations
 ****************************************************************************/
const config = JSON.parse(fs.readFileSync('config.json'));


/*****************************************************************************
 * logger configurations
 ****************************************************************************/
const logger = winston.createLogger({
  transports: [
    new winstonDaily({
      level: 'info',
      datePattern: 'YYYY-MM-DD',
      dirname: './log',
      filename: `%DATE%.log`,
    })
  ],
  format: winston.format.combine(
    winston.format.timestamp({
      format: 'YYYY-MM-DD HH:mm:ss.SSS'
    }),
    winston.format.json()
  ),
  exitOnError: false,
});


/*****************************************************************************
 * socket server configurations
 ****************************************************************************/
const io = new Server(config.port, {
  pingInterval: 3000,
  pingTimeout: 10000
});

logger.info('SERVER STARTUP', { port: config.port });


/*****************************************************************************
 * socket event handlers
 ****************************************************************************/
io.sockets.on('connection', socket => {
  // verify channel
  const channel = config.channels.find(x => x.name === socket.handshake.query.channel);
  if (!channel || (channel.key !== socket.handshake.query.key)) {
    socket.disconnect();
    logger.error('UNAUTHORIZED DEVICE', {
      id: socket.id,
      ip: socket.handshake.headers['x-forwarded-for'],
      channel: socket.handshake.query.channel,
      key: socket.handshake.query.key
    });
    return;
  }

  // TMA-1 device register
  if (socket.handshake.query.device) {
    register_device(socket);
  }

  // telemetry client register
  else if(socket.handshake.query.client) {
    register_client(socket);
  }
});


/*****************************************************************************
 * socket register functions
 ****************************************************************************/
function register_device(socket) {
  socket.join(socket.handshake.query.channel);

  logger.info('DEVICE CONNECTED', {
    id: socket.id,
    ip: socket.handshake.headers['x-forwarded-for'],
    channel: socket.handshake.query.channel
  });

  // emit RTC time fix
  setTimeout(() => socket.emit('rtc_fix', { datetime: dateFormat(new Date(), 'yyyy-mm-dd-HH-MM-ss')}), 1000);

  // on SOCKET_DISCONNECTED
  socket.on('disconnect', reason => {
    logger.info('DEVICE DISCONNECTED', {
      id: socket.id,
      ip: socket.handshake.headers['x-forwarded-for'],
      channel: socket.handshake.query.channel
    });

    socket.broadcast.to('client').emit('socket_lost', { data: reason });
  });

  // on telemetry report
  socket.on('tlog', data => {
    process_telemetry(data, socket);
  });
}

function register_client(socket) {
  socket.join(socket.handshake.query.channel);

  logger.info('CLIENT CONNECTED', {
    id: socket.id,
    ip: socket.handshake.headers['x-forwarded-for'],
    channel: socket.handshake.query.channel
  });
}


/*****************************************************************************
 * telemetry report handler
 ****************************************************************************/
function process_telemetry(data, socket) {
  // corrupt telemetry data
  if (!('log' in data)) {
    return;
  }

  data = translate(data.log.match(/.{2}/g).map(x => parseInt(x, 16)));

  if (data instanceof Error) {
    logger.error('PARSE FAILED', {
      id: socket.id,
      ip: socket.handshake.headers['x-forwarded-for'],
      channel: socket.handshake.query.channel,
      data: data
    });

    return;
  }

  logger.info('REPORT', {
    id: socket.id,
    ip: socket.handshake.headers['x-forwarded-for'],
    channel: socket.handshake.query.channel
    data: data
  });

  socket.broadcast.to(socket.handshake.query.channel).emit('report', { data: data });
}
