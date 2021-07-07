import requests
from datetime import datetime
from flask import Flask, jsonify, request
from prometheus_flask_exporter import PrometheusMetrics

app = Flask(__name__)
metrics = PrometheusMetrics(app)

@app.route('/')
def index():
  return "Hello world!" 

@app.route('/hereisdata', methods=['POST'])                                                                                                    
def add():                                                                                                                              
    data = request.get_json()
    # ... do your business logic, and return some response
    # e.g. below we're just echo-ing back the received JSON data

    json_object = jsonify(data)

    f = open("outputfile.txt", "a")
    f.write(datetime.now().__str__())
    f.write(data.__str__())
    f.write("\n")
    f.close()

    # First check if its a valid request
    api_key = data['api_key']
    if(api_key == ""):
      mac_address = data['macadd']
      
      if(mac_address != ""):
        # collect other data that has come
        # 
        temphumid = data['temphumid']
        if(temphumid != ""):
          tempC = data['temphumid']['tempc']
          humidity = data['temphumid']['humidity']
          heatindex = data['temphumid']['heatic']

          met_temp = "temperature_" + mac_address + "_C"
          tempinfo = metrics.info(met_temp,'Temperature in C')
          tempinfo.set(tempC)
          
          met_humi = "humidity_" + mac_address + "_info"
          humidinfo = metrics.info(met_humi,'Humidity')
          humidinfo.set(humidity)
          
          met_hinde = "heatindex_" + mac_address + "_C"
          heatindexinfo = metrics.info(met_hinde,'Heat Index in C')
          heatindexinfo.set(heatindex)                

    # Create a success json to reply
    responseset = {"Status": "Ok"}
    myresponse = jsonify(responseset);

    return myresponse

if __name__ == '__main__':
    app.run()