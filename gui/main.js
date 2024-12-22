const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('node:path')
const net = require('node:net')

const PIPE_PATH = process.platform === 'win32' ? '\\\\.\\pipe\\VolumeControllerPipe' : '/tmp/VolumeControllerPipe'

let client
let pipeConnected = false
let pipeData = ''

const createWindow = () => {
    const win = new BrowserWindow({
      width: 800,
      height: 600,
      webPreferences: {
        preload: path.join(__dirname, 'preload.js')
      }
    })
  
    win.loadFile('index.html')
}

function handleConnectPipe(event) {
    if (pipeConnected) return

    client = net.connect(PIPE_PATH, () => {
        console.log('Connected to server')
        pipeConnected = true
    })
    
    client.on('data', (data) => {
        console.log('Received:', data.toString())
        pipeData = data.toString()
    })
    
    client.on('end', () => {
        console.log('Disconnected from server')
        pipeConnected = false
    })

    client.on('error', () => {
        console.log('Error occured in pipe communication')
    })
}

function handleGetConfig() {
    return pipeData;
}

function handleRequestConfig() {
    requestConfig()
}

app.whenReady().then(() => {
    ipcMain.on('gui:connectPipe', handleConnectPipe)
    ipcMain.on('gui:requestConfig', handleRequestConfig)
    
    ipcMain.handle('gui:getConfig', handleGetConfig)

    

    createWindow()
})



function requestConfig() {
    if (!pipeConnected) return
    client.write('{"request": "config"}')
}