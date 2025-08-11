FROM node:22-alpine

WORKDIR /home/node/monolith
COPY server/package*.json ./
COPY server .
COPY web/assets/types.js ../web/assets/types.js

RUN npm install

EXPOSE 9512
CMD [ "node", "index.mjs" ]
