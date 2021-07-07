import requests
from datetime import datetime
from flask import Flask, jsonify, request
# from prometheus_flask_exporter import PrometheusMetrics
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

token = "K7AhuPaQQDTFPp5BibprS_ZwpQMBmbcQcrIbaSvyT2b7XKpf_WlfsEemT2ES2NSrh5IF53ZtJuuibC8MFZoCyQ=="
org = "home_iot_project"
bucket = "iot"

client = InfluxDBClient(url="http://localhost:8086", token=token)

app = Flask(__name__)
# metrics = PrometheusMetrics(app)

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

    write_api = client.write_api(write_options=SYNCHRONOUS)

    # write_api.write(bucket, org, json_object)
    mac_address = None
    temperaturesensor_data = None
    tempT = None
    tempC = None
    tempF = None
    tempH = None
    tempHC = None
    tempHF = None

    dustsensor_data = None
    gassensor_data = None

    # First collect all the data that we want to put
    try:
      mac_address = data['macadd']
    except ValueError:
      app.logger.error('error getting MAC Address')

    try:
      temperaturesensor_data = data['temphumid']
    except ValueError:
      app.logger.info('No Temperature Data')

    if(mac_address != None):
      if(temperaturesensor_data != None):
        try:
          tempT = data['temphumid']['type']
          tempC = data['temphumid']['tempc']
          tempF = data['temphumid']['tempf']
          tempH = data['temphumid']['humidity']
          tempHC = data['temphumid']['heatic']
          tempHF = data['temphumid']['heatif']
        except ValueError:
          app.logger.info('No Temperature Data values') 

    currenttime = datetime.utcnow()
    try: 
      if (tempT != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsType", tempT).time(currenttime, WritePrecision.NS))

      if (tempC != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsTempC", float(tempC)).time(currenttime, WritePrecision.NS))

      if (tempF != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsTempF", float(tempF)).time(currenttime, WritePrecision.NS))

      if (tempH != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsHumi", float(tempH)).time(currenttime, WritePrecision.NS))

      if (tempHC != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsHtC", float(tempHC)).time(currenttime, WritePrecision.NS))

      if (tempHF != None):
        write_api.write(bucket, org, Point("temperature").tag("host", mac_address).field("tsHtF", float(tempHF)).time(currenttime, WritePrecision.NS))
    except ValueError:
      app.logger.error('Problem writing to Database') 

    # Create a success json to reply
    responseset = {"Status": "Ok"}
    myresponse = jsonify(responseset);

    return myresponse

if __name__ == '__main__':
    app.run()