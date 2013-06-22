--TEST--
Test if data packet is sent
--SKIPIF--
<?php
if (!extension_loaded('vstats')) {
    die('SKIP The VStats extension is not loaded.');
}
--FILE--
<?php
$status = vstats_send("test_label", array("count" => 1));
var_dump($status);
--EXPECT--
bool(true)