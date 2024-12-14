const btnConfig = document.getElementById('btn-open-config');
const btnCloseConfig = document.getElementById('btn-dialog-close');
const configDialog = document.getElementById('config-dialog');

btnConfig.addEventListener('click', () => {
    configDialog.classList.add("shown")
});

btnCloseConfig.addEventListener('click', () => {
    configDialog.classList.remove("shown");
});

const addBtns = document.getElementsByClassName('addbtn');
for (btn of addBtns) {
    btn.addEventListener('click', (event) => {
        let btn = event.currentTarget;
        if (btn.classList.contains('selected'))
            btn.classList.remove('selected');
        else {
            let peers = btn.parentElement.getElementsByClassName('addbtn');
            for (peer of peers)
                peer.classList.remove('selected');
            btn.classList.add('selected');
        }
    });
}


const dialogSessionList = document.getElementsByClassName('dialog-session-list')[0];

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
    
    for (let i = 1; i <= 6; i++) {
        spanBtns.appendChild(makeAddBtn(i, defaultChannel == i));
    }

    dialogSessionList.appendChild(li);
}

function makeAddBtn(i, selected) {
    let a = document.createElement('a');
    a.href = '#';
    a.classList.add('addbtn');
    a.classList.add('text');
    a.innerText = String(i);
    if (selected) a.classList.add('selected');
    a.addEventListener('click', (event) => {
        let btn = event.currentTarget;
        if (btn.classList.contains('selected'))
            btn.classList.remove('selected');
        else {
            let peers = btn.parentElement.getElementsByClassName('addbtn');
            for (peer of peers)
                peer.classList.remove('selected');
            btn.classList.add('selected');
        }
    });
    return a;
}


addSessionItem('master', 1);
addSessionItem('system', 2);
addSessionItem('chrome.exe', 3);
addSessionItem('javaw.exe', 5);
addSessionItem('Spotify.exe', 6);
addSessionItem('vlc.exe', 2);
addSessionItem('Discord.exe', 4);



document.getElementById('btn-cancel-config').addEventListener('click', () => addSessionItem('asd'));