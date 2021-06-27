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

    # Ouptut the object and 
    print(json_object)


    # Create a success json to reply
    responseset = {"Status": "Ok"}
    myresponse = jsonify(responseset);

    return myresponse

if __name__ == '__main__':
    app.run()