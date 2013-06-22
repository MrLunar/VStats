--TEST--
Test if VStats can set the label prefix
--SKIPIF--
<?php
if (!extension_loaded('vstats')) {
    die('SKIP The VStats extension is not loaded.');
}
--FILE--
<?php
ini_set('vstats.default_prefix', ''); // Always reset to default
$status = vstats_set_prefix("new_prefix");
var_dump($status);
$status = vstats_set_prefix("new_prefix", true);
var_dump($status);
--EXPECT--
bool(true)
bool(true)