<?php

//vstats_set_prefix("dev01-01", true);
//ini_set("vstats.send_request_uuid", false);
//ini_set("vstats.send_request_peak_mem_usage", false);
//ini_set("vstats.send_request_time", true);

$array = array();
$rand_num = rand(100, 50000);
for ($i = 0; $i < $rand_num; $i++)
  $array[] = $i;

$rand_num = rand(1, 5);
sleep($rand_num);

echo "VStats Enabled? ";
var_dump(vstats_enabled());

$stat = array('count' => 35, 69 => 96);
echo "VStats Send Result: ";
var_dump(vstats_send("first_counter", $stat));
