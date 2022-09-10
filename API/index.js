const fs = require('fs');
const http = require('http');
// const https = require('https');
const express = require('express');
const cors = require('cors');
const path = require("path");
var bodyParser = require('body-parser');
const { Sequelize } = require('sequelize');
const moment = require('moment');


// 連線資料庫
const sequelize = new Sequelize('test_ucl', 'root', 'aimo', {
    host: '127.0.0.1',
    dialect: 'mysql'
});

const hostname = '127.0.0.1';
const httpport = 5000;
// const httpsport = 443;

// 讀取網站憑證檔
// const cert = fs.readFileSync('./ssl/sever.pem');
// const key = fs.readFileSync('./ssl/private.key');
// const httpsoptions = {
//     cert: cert,
//     key: key
// };

const app = express();
const httpServer = http.createServer(app);
// const httpsServer = https.createServer(httpsoptions, app);

// http自動轉址
// app.use((req, res, next) => {
//     if (req.protocol === 'http') {
//         res.redirect(301, `https://${hostname}${req.url}`);
//     }
//     next();
// });

app.use(cors());

app.use(express.static("./build"));

// app.get("*", (req, res) => {
//     res.sendFile(
//         path.join(__dirname, "build/index.html")
//     );
// });

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));

app.post("/mqtt", function (req, res) {
    try {
        // console.log(typeof req.body.message); // string
        console.log(req.body.topic & req.body.message);
        mqtt_publish.mqtt_publish(req.body.topic, req.body.message);
        res.json("OK");
    } catch (error) {
        console.log(error);
    }
});

app.post("/addinfo", function (req, res) {
    const Info = sequelize.define(
        'smart_rc_mqtt',
        {
            "id": {
                type: Sequelize.INTEGER,
                autoIncrement: true,
                primaryKey: true
            },
            "rawData": {
                type: Sequelize.STRING,
                defaultValue: "0",
            },
            "save_timestamp": {
                type: Sequelize.DATE,
                defaultValue: Sequelize.NOW,
            }
        },
        {
            modelName: 'smart_rc_mqtt',
            freezeTableName: true,
            timestamps: false,
        }
    );
    var reqBody = req.body;
    sequelize.sync().then(() => {
        // 寫入對映欄位名稱的資料內容
        Info.create({
            // 記得 value 字串要加上引號
            timeStep: moment().format("YYYY-MM-DD HH:mm:ss"),
            rawData: reqBody.rawData,
        }).then(() => {
            // 執行成功後會印出文字
            res.send('successfully created!!');
        }).catch((err) => {
            console.log(err)
        });
    });
});

app.post("/info", function (req, res) {
    try {
        Info().then((data) => {
            res.json(data)
        });
    } catch (error) {
        console.log(error);
    }
});

httpServer.listen(httpport, hostname);
// httpsServer.listen(httpsport, hostname);

console.log('RUN 127.0.0.1:5000')

function Info() {
    const InfoView = sequelize.define(
        'Info_view',
        {
            "id": {
                type: Sequelize.INTEGER,
                autoIncrement: true,
                primaryKey: true
            },
            "rawData": {
                type: Sequelize.STRING,
                // allowNull defaults to true
            },
            "timeStep": {
                type: Sequelize.DATE,
                defaultValue: Sequelize.NOW,
            }
        },
        {
            modelName: 'Info_view',
            freezeTableName: true,
            timestamps: false,
        }
    );

    return new Promise((resolve, reject) => {
        const whitch_info = require('./info')

        sequelize.sync().then(() => {
            InfoView.findAll({
                order: [["id", "DESC"]], limit: 1,
            }).then((cars) => {
                let mytime = cars[0].timeStep;
                let myinfo = cars[0].rawData;
                whitch_info.whitch_info(JSON.parse(myinfo)).then((val) => {
                    resolve({ "Status": "Ok", "val": val, "time": mytime })
                });
            }).catch((err) => {
                console.log("Find_Data查詢失敗")
                console.log("err:", err)
                reject({ "Status": "Failed" })
            });
        });
    })
}