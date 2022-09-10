import logo from './logo.svg';
import './App.css';
import React from 'react';
import Button from 'react-bootstrap/Button';


class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = { timestamp: "" };
  }

  tick() {
    //new Promise((resolve, reject) => {
    fetch('http://127.0.0.1/info', {
      method: "POST",
      headers: { "Content-Type": "application/json", 'Accept': 'application/json', },
    })
      .then((res) => res.json())
      .then((data) => {
        // console.log(data.val);
        console.log(JSON.stringify(data, 0, 4));
        this.setState(() => ({
          battery: data.val
        }));
      })
      .catch((error) => {
        console.log(`Error: ${error}`);
      });
    //})
  }

  componentDidMount() {
    this.interval = setInterval(() => this.tick(), 5000);
  }

  componentWillUnmount() {
    clearInterval(this.interval);
  }

  render() {
    var enableNotifications = document.querySelectorAll('.enable-notifications');
    if ('Notification' in window) {
      for (var i = 0; i < enableNotifications.length; i++) {
        enableNotifications[i].style.display = 'inline-block';
        enableNotifications[i].addEventListener('click', askForNotificationPermission);
      }
    }
    function askForNotificationPermission() {
      Notification.requestPermission(function (status) {
        console.log('User Choice', status);
        if (status !== 'granted') {
          console.log('推播允許被拒絕了!');
        } else {

        }
      });
    }
    return (
      <div className="App" >
        <h3>請開啟網頁通知</h3>{' '}
        <Button variant="outline-primary" className='enable-notifications'>點擊開啟</Button>
      </div>
    );
  }
}




export default App;
