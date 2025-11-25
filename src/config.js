Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('https://aveao.github.io/Pebble-App-Config-Page-Backup/archive/sendow.github.io/pebble/simplicity/config/config_557d63dba404afe20200000a_basalt.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    var config_data = JSON.parse(decodeURIComponent(e.response));
    console.log('Config window returned: ', JSON.stringify(config_data));

    var dict = {
      'DAY_OF_WEEK_ENABLED': config_data.dayOfWeekEnabled ? 1 : 0
    };

    Pebble.sendAppMessage(dict, function(){
      console.log('Sent config data to Pebble');  
    }, function() {
      console.log('Failed to send config data!');
    });
  }
});
