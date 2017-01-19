require 'rubygems'
require 'minitest/autorun'

require 'pry'
require 'rye'

def example()
  task = {} 
  cmd  = {} 

  path = `pwd`.chomp

  rbox = Rye::Box.new("127.0.0.1", :password_prompt=>false, 
                      :info=>STDERR, 
                      #:debug => STDERR, 
                      :paranoid => Net::SSH::Verifiers::Null.new
                     )
  rbox.disable_safe_mode


  # inline
  #Rye::Cmd.add_command :ls, "ls"
  #puts rbox.ls
  # @task = Thread.new() { Thread.current[:output] = @rbox.perf}

  # via class
  # @idle_machine   = Thread.new() {  Thread.current[:output] = self.start; self.wait_idle}

  # in thread
  cmd[:server] = path + "/build/log_server"
  cmd[:client] = path + "/build/log_test_client"

  task[:server]  = Thread.new() {  
    puts "starting thread with #{cmd[:server]}\n"
    Thread.current[:output] = rbox.execute cmd[:server]
  }

  sleep(1)

  task[:client]  = Thread.new() {  
    puts "starting thread with #{cmd[:client]}\n"
    Thread.current[:output] = rbox.execute cmd[:client]
  }

  puts
  task.each_pair {|name,tsk| puts "task #{name} status   = #{tsk.status}\n"}

  puts
  task.each_pair {|name,tsk| puts "task #{name} is alive = #{tsk.alive?}\n"}

  # task.join # Wait for tranfer to complete
  puts
  task.each_pair {|name,tsk| puts "joining task #{name}"; tsk.join; puts "... done\n"}

  # dump output
  puts
  task.each_pair {|name,tsk| puts "\ntask #{name} output =\n"; puts tsk[:output]}
end

example()


