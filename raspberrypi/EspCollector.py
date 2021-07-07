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
    api_key = data['MY_API_KEY']
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
          tempinfo = metrics.info('temperature_info','Temperature in C',mac=mac_address)
          tempinfo.set(tempC)
          humidinfo = metrics.info('humidity_info','Humidity',mac=mac_address)
          humidinfo.set(humidity)
          heatindexinfo = metrics.info('heatindex_info','Heat Index in C',mac=mac_address)
          heatindexinfo.set(heatindex)                

    # Create a success json to reply
    responseset = {"Status": "Ok"}
    myresponse = jsonify(responseset);

    return myresponse

if __name__ == '__main__':
    app.run()