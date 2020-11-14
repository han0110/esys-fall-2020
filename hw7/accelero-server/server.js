const net = require('net')
const http = require('http')
const socketIo = require('socket.io')
const ss = require('socket.io-stream')
const Koa = require('koa')
const koaStatic = require('koa-static')

const config = {
  dev: process.env.NODE_ENV !== 'production',
  tcpPort: process.env.TCP_PORT || 8080,
  httpPort: process.env.HTTP_PORT || 8081,
}

const serveHttp = () => {
  const app = new Koa()
  app.use(koaStatic(`${__dirname}/public`))
  app.use(koaStatic(`${__dirname}/build`))

  return http.createServer(app.callback()).listen(config.httpPort, () => {
    console.log(`http server listen on port ${config.httpPort}`)
  })
}

const serveWs = (httpServer) => {
  let tcpSocket
  const io = socketIo(httpServer).of('/accelero')
  io.on('connection', (socket) => {
    console.log(`ws socket ${socket.id} connected`)

    if (tcpSocket) {
      const stream = ss.createStream()
      ss(socket).emit('data', stream)
      tcpSocket.pipe(stream)
    }

    socket.on('disconnect', () => {
      console.log(`ws socket ${socket.id} disconnected`)
    })
  })

  const tcpServer = net.createServer()
  tcpServer.on('connection', (socket) => {
    console.log('tcp socket connected')
    tcpSocket = socket
    socket.on('close', () => {
      console.log('tcp socket disconnected')
    })
  })

  tcpServer.listen(config.tcpPort, () => {
    console.log(`tcp server listen on port ${config.tcpPort}`)
  })
}

const main = async () => {
  const httpServer = serveHttp()
  serveWs(httpServer)
}

main().catch(console.error)
