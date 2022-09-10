var request = require('request')
const mqtt = require('mqtt');
const { json } = require('body-parser');

const client = mqtt.connect('mqtt://mqtt.54ucl.com', { username: "flask", password: "flask" });

client.on('connect', function () {
    try {
        const info = "demo/SmartRC/down/"
        client.subscribe(info, function (err) {
            if (!err) {
                console.log("subscribe topic:" + info)
            }
        });
    } catch (err) {
        console.log("MQTT 訂閱失敗");
        console.log("err:", err);
    }
});

client.on('message', (topic, message) => {
    try {
        msg = message.toString();
        console.log("topic: " + topic, " msg: ", msg)

        switch (topic) {
            case 'demo/SmartRC/down/':
                console.log('call Info API')
                AddInfoAPI(msg)
                break

        }
        console.log('No handler for topic %s', topic)
    } catch (err) {
        console.log("MQTT 消息接收失敗")
        console.log("err:", err)
    }
})

var Api_Url = "http://127.0.0.1:5000";

function AddInfoAPI(mqtt_content) {
    console.log(typeof (mqtt_content))
    let reqbody = {
        rawData: mqtt_content,
    }
    console.log("reqbody:" + JSON.stringify(reqbody))
    request.post({
        headers: { 'content-type': 'application/json' },
        url: Api_Url + '/addinfo',
        body: reqbody,
        json: true
    }, function (error, response, body) {
        console.error('error:', error, "error End");
        console.log('statusCode:', response && response.statusCode);
        console.log('body:', body);
    })
}