socket = io.connect("/", { query: { client: true } });

socket.on('connect', () => {
  console.log("connect!");
});

socket.on('connect_error', () => {
  console.log("error!");
});
