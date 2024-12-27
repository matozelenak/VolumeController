const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('versions', {
    node: () => process.versions.node,
    chrome: () => process.versions.chrome,
    electron: () => process.versions.electron,

    // getConfig: () => ipcRenderer.invoke('gui:getConfig'),
    connectPipe: () => ipcRenderer.send('gui:connectPipe'),
    requestConfig: () => ipcRenderer.send('gui:requestConfig'),
    saveConfig:(config) => ipcRenderer.send('gui:saveConfig', config),
    isBackendConnected: () => ipcRenderer.invoke('gui:isBackendConnected'),
    isControllerConnected: () => ipcRenderer.invoke('gui:isControllerConnected'),

    onPipeData: (callback) => ipcRenderer.on('main:pipeData', (_event, pipeData) => callback(pipeData)),
    onPipeStatus: (callback) => ipcRenderer.on('main:backendStatus', (_event, backendStatus) => callback(backendStatus))
    
})