import json
import string
import webserver

class RobonomicsSender
  var seconds_from_last_send
  var delay
  var delay_mins
  var last_tx_time
  var last_tx_result
  var value_saved
  
  def init()
    self.delay_mins = 10
    self.delay = self.delay_mins * 60
    self.seconds_from_last_send = 0
    self.last_tx_time = "nil"
    self.last_tx_result = "nil"
    self.value_saved = false
  end
    
  def every_second()
    self.seconds_from_last_send = self.seconds_from_last_send + 1
    if self.seconds_from_last_send > self.delay
      self.seconds_from_last_send = 0
      var energy_string = self.read_data()
      var tasmota_command = string.format("SendDatalog %s", energy_string)
      var res = tasmota.cmd(tasmota_command)
      self.last_tx_result = res.find("SendDatalog")
      self.last_tx_time = tasmota.time_str(tasmota.rtc().find("utc"))
    end
  end
  
  def read_data()
    var res = energy.read()
    var energy_data = {}
    energy_data["yesterday_sum"] = res.find("yesterday_sum")
    energy_data["daily"] = res.find("daily")
    energy_data["total"] = res.find("total")
    energy_data["voltage"] = res.find("voltage")
    energy_data["current"] = res.find("current")
    energy_data["active_power"] = res.find("active_power")
    var energy_string = json.dump(energy_data)
    return energy_string
  end

  def web_add_handler()
    webserver.on("/robsend", / -> self.handle_custom_page())
  end
  
  def handle_custom_page()
    if (!webserver.check_privileged_access())
      return
    end

    self.value_saved = false
  
    if webserver.arg_size() > 0 && webserver.has_arg("send_delay")
      self.delay_mins = int(webserver.arg("delay"))
      self.delay = self.delay_mins * 60
      self.value_saved = true
    end
  
    webserver.content_start("Robonomics Sending Config")
    webserver.content_send_style()
    webserver.content_send("<h2>Robonomics Sending Config</h2>")
    webserver.content_send("<form action='/robsend' method='get'>")
    webserver.content_send("<p><b>Enter Delay (in minutes):</b><br><input type='number' name='delay' value='" + str(self.delay_mins) + "' min='0' step='1'></p>")
    if (self.value_saved)
      webserver.content_send("<p><button type='submit' name='send_delay' value='1' style='background-color: green; color: white;' disabled>Saved</button></p>")
    else
      webserver.content_send("<p><button type='submit' name='send_delay' value='1'>Save</button></p>")
    end
    webserver.content_send("</form>")
    webserver.content_send("<p><b>Last Transaction Time:</b> " + self.last_tx_time + "</p>")
    webserver.content_send("<p><b>Last Transaction Result:</b> " + self.last_tx_result + "</p>")
    webserver.content_button(webserver.BUTTON_MAIN)
    webserver.content_stop()
  end
  
  def web_add_main_button()
    webserver.content_send("<p><form action='/robsend' method='get'><button>Robonomics Sending Config</button></form></p>")
  end
end

robonomics = RobonomicsSender()
tasmota.add_driver(robonomics)