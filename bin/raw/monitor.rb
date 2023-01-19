
 
require "serialport"
require 'rubyserial'
print "\nPress CTRL+C or the X to Close\n"
def seach_ports
  ports = []
  1.upto 64 do |index|
    begin
      serial = Serial.new  portname = 'COM' + index.to_s
      ports << portname if serial
      serial.close
    rescue  Exception => e
      ports << portname if e.to_s.include? "ACCESS_DENIED"
    end
  end
  return ports 
end


while true do

trap "SIGINT" do
  puts "Exiting"
  exit 130
end
 
begin
ports = seach_ports
if ports.empty?
	print "Error or No Available Serial Ports\r"
	raise
end
print "\nAvailable Serial Ports:\n"
p ports
puts "Enter Port"
a = gets
a ||= ''       
a.chomp!
puts "Enter Baud Rate"
b = gets
b ||= ''       
b.chomp!

port_str = a 
baud_rate = b.to_i
data_bits = 8
stop_bits = 1
parity = SerialPort::NONE
  
sp = SerialPort.new(port_str, baud_rate, data_bits, stop_bits, parity)
print "\n>>>>>>>>>>"
print port_str.upcase
print "<<<<<<<<<<\n"
#just read forever

while true
   while (i = sp.gets) do 
      puts i
      #puts i.class #String
    end
end
	sp.close      
rescue

end              
end