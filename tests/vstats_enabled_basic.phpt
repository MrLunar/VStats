--TEST--
Test if VStats is enabled via function
--SKIPIF--
<?php
if (!extension_loaded('vstats')) {
    die('SKIP The VStats extension is not loaded.');
}
--FILE--
<?php
$enabled = vstats_enabled();
var_dump($enabled);
--EXPECT--
bool(true)