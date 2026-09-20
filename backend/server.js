const { Client } = require('pg');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const express = require('express');
const cors = require('cors');

// ==========================================
// 1. KẾT NỐI DATABASE POSTGRESQL
// ==========================================
const db = new Client({
    user: 'postgres',
    host: 'localhost',
    database: 'smart_city',
    password: 'password', // <--- ĐIỀN LẠI PASS DB VÀO ĐÂY
    port: 5432,
});

db.connect()
    .then(() => console.log('✅ Đã kết nối thành công với PostgreSQL!'))
    .catch(err => console.error('❌ Lỗi kết nối DB:', err.stack));

// ==========================================
// 2. KẾT NỐI & CẤU HÌNH LORA (MẠCH THU)
// ==========================================
const port = new SerialPort({ path: 'COM5', baudRate: 115200 });
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

// MỚI: TỰ ĐỘNG GỬI LỆNH AT KHI MỞ CỔNG THÀNH CÔNG
port.on('open', () => {
    console.log('🔌 Đã mở cổng COM5. Đang ra lệnh đánh thức LoRa RX...');
    
    // Gửi lệnh sang chế độ TEST
    port.write("AT+MODE=TEST\r\n");
    
    // Chờ 1 giây cho mạch kịp phản hồi rồi ép sang chế độ Lắng nghe
    setTimeout(() => {
        port.write("AT+TEST=RXLRPKT\r\n");
        console.log('🎧 Đã thiết lập xong chế độ RX! Đang chờ tín hiệu vô tuyến bay về...');
    }, 1000);
});

// Hàm dịch mã HEX sang chữ thường
function hexToAscii(hexString) {
    let str = '';
    for (let i = 0; i < hexString.length; i += 2) {
        str += String.fromCharCode(parseInt(hexString.substr(i, 2), 16));
    }
    return str;
}

// ==========================================
// 3. XỬ LÝ DỮ LIỆU BAY VỀ
// ==========================================
parser.on('data', async (data) => {
    // In trực tiếp mọi thứ LoRa phản hồi ra Terminal để dễ theo dõi
    console.log('LoRa phản hồi:', data);

    if (data.includes('+TEST: RX')) {
        const hexPart = data.match(/"([^"]+)"/);
        
        if (hexPart && hexPart[1]) {
            const decodedStr = hexToAscii(hexPart[1]);
            const distanceMatch = decodedStr.match(/\d+/);
            
            if (distanceMatch) {
                const mucNuoc = parseInt(distanceMatch[0]);
                console.log(`\n💧 Trạm 1 báo về khoảng cách: ${mucNuoc} cm`);
                
                try {
                    const query = 'INSERT INTO water_levels (station_id, depth) VALUES ($1, $2)';
                    await db.query(query, [1, mucNuoc]); 
                    console.log('   -> 💾 Đã lưu vào Database thành công!');
                } catch (error) {
                    console.error('   -> ❌ Lỗi khi lưu DB:', error.message);
                }
            }
        }
    }
});

port.on('error', function(err) {
  console.log('❌ Lỗi cổng COM: ', err.message);
});

// ==========================================
// 4. XÂY DỰNG WEB API (EXPRESS.JS)
// ==========================================
const app = express();
app.use(cors());
app.use(express.json());

// API 1: Lấy lịch sử mực nước mới nhất
app.get('/api/history', async (req, res) => {
    try {
        // Lấy 5 bản ghi mới nhất từ trạm số 1
        const query = 'SELECT depth, recorded_at FROM water_levels WHERE station_id = 1 ORDER BY recorded_at DESC LIMIT 5';
        const result = await db.query(query);
        
        res.json({
            success: true,
            message: "Dữ liệu mực nước trạm 1",
            data: result.rows
        });
    } catch (error) {
        res.status(500).json({ success: false, error: error.message });
    }
});

// API 2: Trả về trạng thái ngập để OSRM bẻ đường (Logic trọng số động)
app.get('/api/status', async (req, res) => {
    try {
        const query = 'SELECT depth FROM water_levels WHERE station_id = 1 ORDER BY recorded_at DESC LIMIT 1';
        const result = await db.query(query);
        
        if (result.rows.length > 0) {
            const currentDepth = result.rows[0].depth;
            let status = 'Xanh'; // Bình thường
            
            if (currentDepth >= 20) status = 'Đỏ';      // Ngập nặng -> Cấm đường
            else if (currentDepth >= 10) status = 'Vàng'; // Ngập nhẹ -> Tăng trọng số
            
            res.json({ station_id: 1, depth: currentDepth, status: status });
        } else {
            res.json({ message: "Chưa có dữ liệu" });
        }
    } catch (error) {
        res.status(500).json({ success: false, error: error.message });
    }
});

// Khởi động Web Server ở cổng 3000
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`🌐 API Server đang chạy tại: http://localhost:${PORT}`);
});