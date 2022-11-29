from flask import Flask, request

app = Flask(__name__)


@app.route('/')
def hello():
    return '<h1>Hello, World!</h1>'

@app.route('/temperature', methods=['POST'])
def temp_value():
    request_json = request.get_json()
    temperature = request_json['temperature']
    humidity = request_json['humidity']
    print(f'Temperature: {temperature}°F')
    print(f'Humidity: {humidity}%')
    return 'Succeeded'

if __name__ == "__main__":
  app.run(host="0.0.0.0", port=3000)