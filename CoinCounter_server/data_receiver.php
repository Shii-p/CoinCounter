<?php
$file = "data_log.txt";
$debug_file = "debug_log.txt";

if (isset($_GET['data'])) {
    $jsonData = $_GET['data'];
    file_put_contents($debug_file, "受信データ (GET): " . $jsonData . "\n", FILE_APPEND | LOCK_EX);
    $decodedJson = urldecode($jsonData);
    $data = json_decode($decodedJson, true);
    if ($data !== null) {
        $formattedData = date("Y-m-d H:i:s") . " - " . json_encode($data, JSON_PRETTY_PRINT) . "\n";
        file_put_contents($file, $formattedData, FILE_APPEND | LOCK_EX);
        http_response_code(200);
        echo "データを保存しました";
    } else {
        http_response_code(400);
        echo "JSON データの解析に失敗しました";
    }
} else {
    http_response_code(400);
    echo "データが受信できませんでした";
}
?>
