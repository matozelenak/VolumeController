const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('versions', {
    node: () => process.versions.node,
    chrome: () => process.versions.chrome,
    electron: () => process.versions.electron,
    getConfig: () => ipcRenderer.invoke('gui:getConfig'),
    connectPipe: () => ipcRenderer.send('gui:connectPipe'),
    requestConfig: () => ipcRenderer.send('gui:requestConfig')
    
})