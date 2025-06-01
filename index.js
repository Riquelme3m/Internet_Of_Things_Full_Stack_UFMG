const express = require("express");
const mqtt = require("mqtt");
const app = express();
const port = 3000;
const http = require(http);
const server = http.createServer(app); //Server will run on port 3000

const path = require("path");

const { Server } = require("socket.io");

const io = new Server(server);

const routes  = require("./src/routes/index");
const sensorModel = require("./src/models/sensors.model");

//setup ejs

app.set("View engine","ejs");


// specify the directory where ejs is located
app.set("views",path.join(__dirname,"public/"));

//a buffer to store sensor data

let sensorBuffer = [];


//middleware to parse JSON data

app.use(express.json());

//add routes

app.use("/api",routes);



//setup MQTT

const mqttClient = mqtt.connect("mqtt://200.238.206.54"); //MQTT client connects to my machine through this IP

//when mqtt is connected

const topics = ["/sensors_monitors","/sensors/esp32/temp","/sensors/esp32/pressure","/sensors/esp32/airQuality","/sensors/esp32/light"];
  

mqttClient.on("connect",()=>{
    console.log("Connected to mqtt broker");

    mqttClient.subscribe(topics,(err)=>{
        if(err){
            console.error("Failed to subscribe to topic",err);
        }else{
            console.log("MQTT Client subscribed to the topics");
        }
    })
})

mqttClient.on("message",async(topic,message)=>{
    if(topic == "/sensors_monitors"){
        const get_payload_str = message.toString();
        console.log(`${topic}: payload string received: ${get_payload_str}`);
        const sensorData = convert_payload_str_to_obj(get_payload_str);
        console.log(sensorData);

        // update the webclient

        io.emit("sensorData",sensorData);

        //save sensor data(average) in database over a period of 5 minutes

        save_avg_sensor_data(sensorData);
    }
})

//define few http endpoints

app.get("/",(req,res)=>{
    console.log("Hello world!");
    res.status(200).send("Data received");
})

app.get("/graph",(req,res)=>{
    console.log("showing graph");
    res.sendFIle
})