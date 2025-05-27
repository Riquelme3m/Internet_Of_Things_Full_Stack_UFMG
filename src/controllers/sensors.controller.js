const sensorService = require("../services/sensors.services");


exports.getAllSensorData = async(req,res) =>{
    try{
        const sensor_data = await sensorService.getAllSensorData;
        res.status(200).json(sensor_data);
    }
    catch(error){
        res.status(500).json({message: error.message});
    }
}

exports.getSensorDataById = async(req,res) => {
    try{

        const sensor_data = await sensorService.getSensorDataById(req.param.id);
        res.status(200).json(sensor_data);
    }
    catch(error){
        res.status(500).json({message:error.message});
    }
}

exports.getSensorDataWithinRange = async (req,res) =>{
    try{
        const sensor_data = await sensorService.getSensorDataWithinRange(req.body);
        res.status(200).json(sensor_data);
    }
    catch(error){
        res.status(500).json({message:error.message})
    }
}

exports.createSensorData = async(req,res) =>{
    try{
        const sensor_data = await sensorService.createSensorData(req.body);
        res.status(200).json(sensor_data);
    }
    catch(error){
        res.status(500).json({message:error.message});
    }
}