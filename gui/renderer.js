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
let sessionPool = undefined;
let sessionPoolCopy = undefined;
let settings = undefined;
let pipeConnected = false;

////////////////////////////////////////////////////
//////////////// status bar buttons ////////////////
////////////////////////////////////////////////////
btnRefresh.addEventListener('click', () => {
    window.versions.requestConfig();
})

btnOpenConfig.addEventListener('click', () => {
    if (settingsDialog.classList.contains('shown')) return;
    configDialog.classList.add('shown');
});

btnSettings.addEventListener('click', () => {
    if (configDialog.classList.contains('shown')) return;
    settingsDialog.classList.add('shown');
});

////////////////////////////////////////////////////
/////////////// config dialog buttons //////////////
////////////////////////////////////////////////////
btnCloseConfig.addEventListener('click', () => {
    configDialog.classList.remove('shown');
    if (!sessionPool) return;
    sessionPoolCopy = JSON.parse(JSON.stringify(sessionPool));
    makeSessionList(sessionPool);  // discard changes when closing dialog
});

btnSaveConfig.addEventListener('click', () => {
    if (!sessionPool) return;
    let data = {};
    data.configFile = {};

    makeConfig(data.configFile, sessionPoolCopy);
    sessionPool = JSON.parse(JSON.stringify(sessionPoolCopy));
    window.versions.saveConfig(JSON.stringify(data));
    configDialog.classList.remove("shown");
    makeChannelBlocks();
});

btnCancelConfig.addEventListener('click', () => {
    configDialog.classList.remove("shown");
    if (!sessionPool) return;
    sessionPoolCopy = JSON.parse(JSON.stringify(sessionPool));
    makeSessionList(sessionPool);  // discard changes when closing dialog
});

btnAddSession.addEventListener('click', () => {
    if (!sessionPool) return;
    const sessionName = inputAddSession.value;
    if (sessionName === '') return;
    inputAddSession.value = '';
    sessionPoolCopy.push({name: sessionName, assignedChannel: -1});
    addSessionItem(sessionName, -1);
});

////////////////////////////////////////////////////
///////////// settings dialog buttons //////////////
////////////////////////////////////////////////////
btnCloseSettings.addEventListener('click', () => {
    settingsDialog.classList.remove('shown');
    if (!settings) return;
    loadSettings(); // discard changes when closing dialog
});

btnSaveSettings.addEventListener('click', () => {
    if (!settings) return;
    let data = {};
    data.configFile = {};
    data.configFile.port = settingPortName.selectedOptions[0].value;
    data.configFile.baud = parseInt(settingBaudRate.value);
    data.configFile.parity = settingComParity.selectedOptions[0].value;
    
    window.versions.saveConfig(JSON.stringify(data));
    settingsDialog.classList.remove("shown");
});

btnCancelSettings.addEventListener('click', () => {
    settingsDialog.classList.remove("shown");
    if (!settings) return;
    loadSettings(); // discard changes when closing dialog
});

////////////////////////////////////////////////////
/////////////// ipc functions //////////////////////
////////////////////////////////////////////////////
window.onload = () => {
    reconnectIntervalID = setInterval(tryReconnectPipe, 1000);
};

window.versions.onPipeData((pipeData) => {
    let jsonPipeData = {};
    try {
        jsonPipeData = JSON.parse(pipeData);
    } catch (error) {
        console.log("Failed to parse JSON: ", error.message);
    }

    if (jsonPipeData.status) {
        const status = jsonPipeData.status;
        const deviceConnected = status.deviceConnected;
        updateDeviceStatus(deviceConnected);
        if (status.comPorts) {
            settingPortName.children = [];
            for (port of status.comPorts) {
                settingsOptionAdd(settingPortName, port);
            }
            if (settings) {
                settingsOptionAdd(settingPortName, settings.port);
                settingOptionSelect(settingPortName, settings.port);
            }
        }
    }

    if (jsonPipeData.configFile || jsonPipeData.sessionPool || jsonPipeData.devicePool) {
        sessionPool = [];
        sessionPoolCopy = [];
        addToSessionPool(sessionPool, 'default output device', -1, false);
        addToSessionPool(sessionPool, 'system sounds', -1, false);
        addToSessionPool(sessionPool, 'other applications', -1, false);
    }

    if (jsonPipeData.configFile) {
        const configFile = jsonPipeData.configFile;
        
        settings = [];
        settings.port = configFile.port;
        settings.baud = configFile.baud;
        settings.parity = configFile.parity;
        loadSettings();
        
        for (channel of configFile.channels) {
            for (sessionName of channel.sessions) {
                const newName = sessionNameFromInternal(sessionName);
                addToSessionPool(sessionPool, newName, channel.id, true);
            }
        }
    }
    if (jsonPipeData.sessionPool) {
        for (sessionName of jsonPipeData.sessionPool) {
            const newName = sessionNameFromInternal(sessionName);
            addToSessionPool(sessionPool, newName, -1, false);
        }
    }
    if (jsonPipeData.devicePool) {
        for (deviceName of jsonPipeData.devicePool) {
            addToSessionPool(sessionPool, deviceName, -1, false);
        }
    }

    if (jsonPipeData.configFile || jsonPipeData.sessionPool || jsonPipeData.devicePool) {
        sessionPoolCopy = JSON.parse(JSON.stringify(sessionPool));
        makeSessionList(sessionPool);
        makeChannelBlocks();
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

////////////////////////////////////////////////////
/////////// settings & config functions ////////////
////////////////////////////////////////////////////
function makeConfig(jsonData, sessionData) {
    jsonData.channels = [];
    for (let i = 0; i <= 5; i++)
        jsonData.channels.push({id: i, sessions: []});

    for (session of sessionData) {
        if (session.assignedChannel == -1) continue;
        const channelSessions = jsonData.channels[session.assignedChannel].sessions;
        channelSessions.push(sessionNameToInternal(session.name));
    }
}

function loadSettings() {
    if (!settings) return;
    settingsOptionAdd(settingPortName, settings.port);
    settingOptionSelect(settingPortName, settings.port);
    settingBaudRate.value = settings.baud;
    settingOptionSelect(settingComParity, settings.parity);
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

function addToSessionPool(sessionPool, sessionName, ch, bChange) {
    if (sessionPool.some(e => {return e.name === sessionName;})) {
        if (!bChange) return;
        sessionPool.find(e => {return e.name === sessionName;}).assignedChannel = ch;
        // console.log(sessionName + ' changed to ' + ch);
    }
    else {
        sessionPool.push({name: sessionName, assignedChannel: ch});
        // console.log(sessionName + ' added to ' + ch);
    }
}

//////////////////////////////////////////////////////////////
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
    for (session of list) {
        addSessionItem(session.name, session.assignedChannel);
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
        const ch = session.assignedChannel;
        if (ch == -1) continue;

        const li = document.createElement('li');
        li.classList.add('app-item');
        li.innerText = sessionNameFromInternal(session.name);

        channelBlocks[ch].appendChild(li);
    }
}

function sessionNameFromInternal(name) {
    switch (name) {
        case 'master':
            return 'default output device';
        case 'system':
            return 'system sounds';
        case 'other':
            return 'other applications';
    }
    return name;
}

function sessionNameToInternal(name) {
    switch (session.name) {
        case 'default output device':
            return 'master';
        case 'system sounds':
            return 'system';
        case 'other applications':
            return 'other';
    }
    return name;
}

