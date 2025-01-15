const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('node:path')
const net = require('node:net')

const PIPE_PATH = process.platform === 'win32' ? '\\\\.\\pipe\\VolumeControllerPipe' : '/tmp/VolumeControllerPipe'

let client
let pipeConnected = false
let pipeData = ''
let controllerConnected = false
let win

const createWindow = () => {
    win = new BrowserWindow({
      width: 800,
      height: 600,
      webPreferences: {
        preload: path.join(__dirname, 'preload.js')
      }
    })
    
    win.menuBarVisible = false;
    win.loadFile('index.html')
}

function handleConnectPipe(event) {
    console.log('trying to connect to pipe...')
    if (pipeConnected) return

    client = net.connect(PIPE_PATH, () => {
        console.log('Connected to server')
        pipeConnected = true
        win.webContents.send('main:backendStatus', pipeConnected)
    })
    
    client.on('data', (data) => {
        console.log('Received:', data.toString())
        pipeData = data.toString()
        win.webContents.send('main:pipeData', pipeData)
    })
    
    client.on('end', () => {
        console.log('Disconnected from server')
        pipeConnected = false
        win.webContents.send('main:backendStatus', pipeConnected)
    })

    client.on('error', () => {
        console.log('Error occured in pipe communication')
    })
}

function handleSaveConfig(event, config) {
    if(!pipeConnected) return;
    client.write(config);
}

// function handleGetConfig() {
//     return pipeData;
// }

app.whenReady().then(() => {
    ipcMain.on('gui:connectPipe', handleConnectPipe)
    ipcMain.on('gui:requestConfig', () => { requestConfig() })
    ipcMain.on('gui:saveConfig', handleSaveConfig)
    
    // ipcMain.handle('gui:getConfig', handleGetConfig)
    ipcMain.handle('gui:isBackendConnected', () => { return pipeConnected })
    ipcMain.handle('gui:isControllerConnected', () => { return controllerConnected })
    

    createWindow()
})



function requestConfig() {
    if (!pipeConnected) return
    pipeData = ''
    console.log('refresh config')
    client.write('{"request": ["status", "config", "sessionPool", "devicePool"]}')
}