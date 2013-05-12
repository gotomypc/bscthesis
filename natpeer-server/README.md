# natpeer-server

The rendezvous server

## Instructions

Required software:

* Node.js and npm (they should come together)
* GCC compiler collection for building native dependencies
* MongoDB

You need to register an application over the
[Google API Console](https://code.google.com/apis/console/) and set up the
Google Cloud Messaging for Android service. After that, enter the API key to
`natpeer-server.js:16`.

Install the dependencies
```
npm install
```

Start the server
```
npm start
```
