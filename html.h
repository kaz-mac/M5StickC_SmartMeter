#pragma once

// TOPページのHTML
static const char HTML_CPANEL[] PROGMEM = R"EOT(
<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { font-family: Arial, sans-serif; padding: 10px; }
  #title { text-align: center; font-size: 36px; margin-bottom: 20px; }
  #buttons { margin-top: 40px; text-align: center; }
  .btn {
    padding: 15px 30px; 
    font-size: 18px; 
    margin: 0 10px; 
    vertical-align: top;
  }
  #watt {
    margin-top: 20px;
    color: black;
    text-align: center;
    font-size: 80px;
  }
  #date {
    margin-top: 20px;
    color: black;
    text-align: center;
    font-size: 24px;
  }
</style>
</head>
<body>

<div id="title">消費電力</div>
<div id="watt">---- W</div>
<div id="date">1970-01-01 00:00:00</div>

<div id="buttons">
  <button class="btn" onclick="update()">更新</button>
</div>

<script>
function update() {
  fetch("/api/status?key=[API_KEY]")
  .then(response => response.json())
  .then(data => {
    document.getElementById('watt').textContent = data.watt + "W";
    document.getElementById('date').textContent = data.date;
  })
  .catch(error => {
    console.error('Error:', error);
  });
}
update();
//setInterval(update, 5100);
</script>

</body>
</html>
)EOT";
