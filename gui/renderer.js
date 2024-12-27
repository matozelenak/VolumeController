const txtBackendStatus = document.getElementById('txt-backend-status');
const txtControllerStatus = document.getElementById('txt-controller-status');
const btnRefresh = document.getElementById('btn-refresh');
const btnOpenConfig = document.getElementById('btn-open-config');
const btnSettings = document.getElementById('btn-settings');

// config dialog
const configDialog = document.getElementById('config-dialog');
const btnCloseConfig = document.getElementById('btn-config-dialog-close');
const btnSaveConfig = document.getElementById('btn-save-config');
const btnCancelConfig = document.getElementById('btn-cancel-config');
const inputAddSession = document.getElementById('input-add-session');
const btnAddSession = document.getElementById('btn-add-session');

// settings dialog
const settingsDialog = document.getElementById('settings-dialog');
const btnCloseSettings = document.getElementById('btn-settings-dialog-close');
const btnSaveSettings = document.getElementById('btn-save-settings');
const btnCancelSettings = document.getElementById('btn-cancel-settings');
const settingPortName = document.getElementById('setting-comport').getElementsByClassName('setting-select')[0];
const settingBaudRate= document.getElementById('setting-baudrate').getElementsByClassName('setting-numinput')[0];
const settingComParity = document.getElementById('setting-comparity').getElementsByClassName('setting-select')[0];


const channelBlocks = [
    document.getElementById('channel-block-1').getElementsByClassName('app-list')[0],
    document.getElementById('channel-block-2').getElementsByClassName('app-list')[0],
    document.getElementById('channel-block-3').getElementsByClassName('app-list')[0],
    document.getElementById('channel-block-4').getElementsByClassName('app-list')[0],
    document.getElementById('channel-block-5').getElementsByClassName('app-list')[0],
    document.getElementById('channel-block-6').getElementsByClassName('app-list')[0]
];

const dialogSessionList = document.getElementsByClassName('dialog-session-list')[0];
let reconnectIntervalID;
let sessionPool = [];
let sessionPoolCopy = [];
let pipeConnected = false;
let jsonPipeData = '';


// status bar buttons
btnRefresh.addEventListener('click', () => {
    window.versions.requestConfig();
})

btnOpenConfig.addEventListener('click', () => {
    configDialog.classList.add('shown');
});

btnSettings.addEventListener('click', () => {
    settingsDialog.classList.add('shown');
});

// config dialog buttons
btnCloseConfig.addEventListener('click', () => {
    configDialog.classList.remove('shown');
});

btnSaveConfig.addEventListener('click', () => {
    if (jsonPipeData === '') return;
    let data = {};
    data.configFile = {};

    makeSettings(data.configFile, jsonPipeData.configFile);
    makeConfig(data.configFile, sessionPoolCopy);
    sessionPool = JSON.parse(JSON.stringify(sessionPoolCopy));
    window.versions.saveConfig(JSON.stringify(data));
    configDialog.classList.remove("shown");
    makeChannelBlocks();
});

btnCancelConfig.addEventListener('click', () => {
    sessionPoolCopy = JSON.parse(JSON.stringify(sessionPool));
    makeSessionList(sessionPool);
    configDialog.classList.remove("shown");
});

btnAddSession.addEventListener('click', () => {
    const sessionName = inputAddSession.value;
    if (sessionName === '') return;
    inputAddSession.value = '';
    sessionPoolCopy.push({name: sessionName, assignedChannel: -1});
    addSessionItem(sessionName, -1);
});

// settings dialog buttons
btnCloseSettings.addEventListener('click', () => {
    settingsDialog.classList.remove('shown');
});

btnSaveSettings.addEventListener('click', () => {
    if (jsonPipeData === '') return;
    let data = {};
    data.configFile = {};
    jsonPipeData.configFile.port = settingPortName.selectedOptions[0].value;
    jsonPipeData.configFile.baud = parseInt(settingBaudRate.value);
    jsonPipeData.configFile.parity = settingComParity.selectedOptions[0].value;
    
    makeSettings(data.configFile, jsonPipeData.configFile);
    makeConfig(data.configFile, sessionPool);
    window.versions.saveConfig(JSON.stringify(data));
    settingsDialog.classList.remove("shown");
});

btnCancelSettings.addEventListener('click', () => {
    settingsDialog.classList.remove("shown");
});

window.onload = () => {
    reconnectIntervalID = setInterval(tryReconnectPipe, 1000);
};

function makeSettings(jsonData, settingsData) {
    jsonData.port = settingsData.port;
    jsonData.baud = settingsData.baud;
    jsonData.parity = settingsData.parity;
}

function makeConfig(jsonData, sessionData) {
    jsonData.channels = [];
    for (let i = 0; i <= 5; i++)
        jsonData.channels.push({id: i, sessions: []});

    for (session of sessionData) {
        if (session.assignedChannel == -1) continue;
        jsonData.channels[session.assignedChannel].sessions.push(session.name);
    }
}

function settingsOptionAdd(select, optionName) {
    for (let i = 0; i < select.children.length; i++) {
        if (select.children[i].value === optionName) {
            return;
        }
    }
    let node = document.createElement('option');
    node.setAttribute('value', optionName);
    node.innerText = optionName;
    select.appendChild(node);
}

function settingOptionSelect(select, optionName) {
    for (let i = 0; i < select.children.length; i++) {
        if (select.children[i].value === optionName) {
            select.selectedIndex = i;
            break;
        }
    }
}


window.versions.onPipeData((pipeData) => {
    try {
        jsonPipeData = JSON.parse(pipeData);
        
        if (jsonPipeData.status) {
            const status = jsonPipeData.status;
            const deviceConnected = status.deviceConnected;
            updateDeviceStatus(deviceConnected);
            if (status.comPorts) {
                settingPortName.children = [];
                for (port of status.comPorts) {
                    settingsOptionAdd(settingPortName, port);
                }
            }
        }

        if (jsonPipeData.configFile || jsonPipeData.sessionPool || jsonPipeData.devicePool) {
            sessionPool = [];
        }

        if (jsonPipeData.configFile) {
            const configFile = jsonPipeData.configFile;
            
            
            settingsOptionAdd(settingPortName, configFile.port);
            settingOptionSelect(settingPortName, configFile.port);
            settingBaudRate.value = configFile.baud;
            settingOptionSelect(settingComParity, configFile.parity);

            for (channel of configFile.channels) {
                for (sessionName of channel.sessions) {
                    if (sessionPool.some(e => {return e.name === sessionName;})) continue;
                    // if (devicePool.some(e => {e.name === sessionName})) continue;
                    sessionPool.push({name: sessionName, assignedChannel: channel.id});
                }
            }
        }
        if (jsonPipeData.sessionPool) {
            for (sessionName of jsonPipeData.sessionPool) {
                if (sessionPool.some(e => {return e.name === sessionName;})) continue;
                sessionPool.push({name: sessionName, assignedChannel: -1});
            }
        }
        if (jsonPipeData.devicePool) {
            for (deviceName of jsonPipeData.devicePool) {
                if (sessionPool.some(e => {return e.name === sessionName;})) continue;
                // if (devicePool.some(e => {e.name === deviceName})) continue;
                sessionPool.push({name: deviceName, assignedChannel: -1});
            }
        }

        // make list
        if (jsonPipeData.configFile || jsonPipeData.sessionPool || jsonPipeData.devicePool) {
            sessionPoolCopy = JSON.parse(JSON.stringify(sessionPool));
            makeSessionList(sessionPool);
        }

        // make channel blocks
        makeChannelBlocks();

    } catch (error) {
        console.log("Failed to parse JSON: ", error.message);
    }
});

window.versions.onPipeStatus((pipeStatus) => {
    updatePipeStatus(pipeStatus);
    if (pipeStatus) {
        clearInterval(reconnectIntervalID);
        reconnectIntervalID = 0;
        window.versions.requestConfig();
    }
    else {
        if (reconnectIntervalID == 0)
            reconnectIntervalID = setInterval(tryReconnectPipe, 1000);
    }
});

function tryReconnectPipe() {
    window.versions.connectPipe();
}

function updatePipeStatus(status) {
    pipeConnected = status;
    txtBackendStatus.innerText = status ? 'connected' : 'not connected';
    txtBackendStatus.style.color = status ? 'green' : 'red';
}

function updateDeviceStatus(status) {
    txtControllerStatus.innerText = status ? 'connected' : 'not connected';
    txtControllerStatus.style.color = status ? 'green' : 'red';
}



function makeSessionList(list) {
    dialogSessionList.innerHTML = "";
    for (sessionObj of list) {
        addSessionItem(sessionObj.name, sessionObj.assignedChannel);
    }
}

function addSessionItem(name, defaultChannel) {
    let li = document.createElement('li');
    li.classList.add('session-item');
    let spanName = document.createElement('span');
    spanName.classList.add('session-name');
    spanName.innerText = name;
    li.appendChild(spanName);
    let spanBtns = document.createElement('span');
    spanBtns.classList.add('session-ch-btns');
    li.appendChild(spanBtns);
    
    for (let i = 0; i <= 5; i++) {
        spanBtns.appendChild(makeAddBtn(i, defaultChannel == i, name));
    }

    dialogSessionList.appendChild(li);
}

function makeAddBtn(i, selected, sessionName) {
    let a = document.createElement('a');
    a.href = '#';
    a.classList.add('addbtn');
    a.classList.add('text');
    a.innerText = String(i+1);
    a.setAttribute('sessionName', sessionName);
    a.setAttribute('value', i);
    if (selected) a.classList.add('selected');
    a.addEventListener('click', (event) => {
        let btn = event.currentTarget;
        const sessName = btn.getAttribute('sessionName');
        const val = btn.getAttribute('value');
        if (btn.classList.contains('selected')) {
            btn.classList.remove('selected');
            updateSessionChannel(sessName, -1);
        }
        else {
            let peers = btn.parentElement.getElementsByClassName('addbtn');
            for (peer of peers)
                peer.classList.remove('selected');
            btn.classList.add('selected');
            updateSessionChannel(sessName, val);
        }
    });
    return a;
}

function updateSessionChannel(name, ch) {
    for (session of sessionPoolCopy) {
        if (session.name === name) {
            session.assignedChannel = parseInt(ch);
            break;
        }
    }
}

function makeChannelBlocks() {
    for (block of channelBlocks) {
        block.innerHTML = '';
    }
    for (session of sessionPool) {
        const li = document.createElement('li');
        li.classList.add('app-item');
        li.innerText = session.name;
        
        const ch = session.assignedChannel;
        channelBlocks[ch].appendChild(li);
    }
}

