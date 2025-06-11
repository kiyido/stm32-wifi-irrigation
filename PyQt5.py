import sys
import socket
import threading
import time
import datetime
import pandas as pd
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QLineEdit, QTextEdit, QGroupBox, QFileDialog
)
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QFont
import pyqtgraph as pg


class IrrigationUI(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("WiFi 远程灌溉上位机")
        self.resize(1600, 1000)

        self.sock = None
        self.sock_lock = threading.Lock()
        self.stop_event = threading.Event()
        self.recv_thread = None

        self.data_history = {
            'temp': [],
            'humi': [],
            'light': [],
            'soil': [],
            'time': [],
            'time_str': []
        }

        self.connection_status = False
        self.mode_flag = 0  # 默认手动模式
        self.pump_flag = 0
        self.threshold = 0

        self.init_ui()
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(1000)

    def init_ui(self):
        main_layout = QVBoxLayout(self)

        # === 功能区 ===
        control_group = QGroupBox("功能区")
        control_layout = QHBoxLayout()

        self.ip_input = QLineEdit("192.168.4.1")
        self.port_input = QLineEdit("8080")

        self.connect_button = QPushButton("开始连接")
        self.connect_button.clicked.connect(self.connect_to_esp)

        self.disconnect_button = QPushButton("断开连接")
        self.disconnect_button.clicked.connect(self.disconnect_from_esp)

        self.pump_on_button = QPushButton("开启水泵")
        self.pump_on_button.clicked.connect(lambda: self.send_command("#OPEN#"))

        self.pump_off_button = QPushButton("关闭水泵")
        self.pump_off_button.clicked.connect(lambda: self.send_command("#CLOSE#"))

        self.auto_mode_button = QPushButton("自动模式")
        self.auto_mode_button.clicked.connect(lambda: self.send_command("#AUTO#"))

        self.manual_mode_button = QPushButton("手动模式")
        self.manual_mode_button.clicked.connect(lambda: self.send_command("#MANUAL#"))

        self.threshold_input = QLineEdit()
        self.send_threshold_button = QPushButton("设置阈值")
        self.send_threshold_button.clicked.connect(self.send_threshold)

        self.save_data_button = QPushButton("保存数据")
        self.save_data_button.clicked.connect(self.save_data)

        self.load_data_button = QPushButton("加载数据")
        self.load_data_button.clicked.connect(self.load_data)

        control_layout.addWidget(QLabel("IP:"))
        control_layout.addWidget(self.ip_input)
        control_layout.addWidget(QLabel("端口:"))
        control_layout.addWidget(self.port_input)
        control_layout.addWidget(self.connect_button)
        control_layout.addWidget(self.disconnect_button)
        control_layout.addWidget(self.pump_on_button)
        control_layout.addWidget(self.pump_off_button)
        control_layout.addWidget(self.auto_mode_button)
        control_layout.addWidget(self.manual_mode_button)
        control_layout.addWidget(QLabel("阈值:"))
        control_layout.addWidget(self.threshold_input)
        control_layout.addWidget(self.send_threshold_button)
        control_layout.addWidget(self.save_data_button)
        control_layout.addWidget(self.load_data_button)
        control_group.setLayout(control_layout)

        # === 日志与数据区 ===
        info_layout = QHBoxLayout()

        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)

        self.data_display = QLabel("温度: -- ℃  湿度: -- %  光照: -- lx  土壤湿度: -- %")
        self.status_display = QLabel("模式: 自动模式  水泵: 关闭  阈值: --")

        display_layout = QVBoxLayout()
        display_layout.addWidget(self.data_display)
        display_layout.addWidget(self.status_display)

        info_layout.addWidget(self.log_output, 2)
        info_layout.addLayout(display_layout, 1)

        # === 绘图区 ===
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.addLegend()
        self.temp_curve = self.plot_widget.plot(pen='r', name='温度')
        self.humi_curve = self.plot_widget.plot(pen='b', name='湿度')
        self.light_curve = self.plot_widget.plot(pen='g', name='光照')
        self.soil_curve = self.plot_widget.plot(pen='m', name='土壤')

        # 设置横坐标为字符串（时间标签）
        ax = self.plot_widget.getAxis('bottom')
        ax.setLabel(text='时间')
        ax.setTickSpacing(levels=[(1, 0)])  # 提供一些间隔控制
        ax.setStyle(tickTextOffset=10)
        self.plot_widget.getAxis('bottom').setTicks([[]])  # 初始清空

        main_layout.addWidget(control_group)
        main_layout.addLayout(info_layout)
        main_layout.addWidget(self.plot_widget)

        self.update_ui_state()  # 初始化按钮状态

    def log(self, message):
        timestamp = datetime.datetime.now().strftime("[%H:%M:%S]")
        self.log_output.append(f"{timestamp} {message}")

    def connect_to_esp(self):
        ip = self.ip_input.text()
        port = int(self.port_input.text())
        self.stop_event.clear()
        threading.Thread(target=self._connect_thread, args=(ip, port), daemon=True).start()

    def disconnect_from_esp(self):
        self.stop_event.set()
        with self.sock_lock:
            if self.sock:
                try:
                    self.sock.shutdown(socket.SHUT_RDWR)
                    self.sock.close()
                    self.sock = None
                    self.connection_status = False
                    self.log("[!] 已断开连接")
                except Exception as e:
                    self.log(f"[!] 断开连接失败: {e}")
            else:
                self.log("[!] 尚未连接")
        self.update_ui_state()

    def _connect_thread(self, ip, port):
        try:
            new_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_sock.connect((ip, port))
            with self.sock_lock:
                self.sock = new_sock
            self.connection_status = True
            self.log(f"[+] 已连接 ESP8266: {ip}:{port}")
            self.recv_thread = threading.Thread(target=self.recv_loop, daemon=True)
            self.recv_thread.start()
        except Exception as e:
            self.connection_status = False
            self.log(f"[!] 连接失败: {e}")
        self.update_ui_state()

    def recv_loop(self):
        try:
            while not self.stop_event.is_set():
                with self.sock_lock:
                    if not self.sock:
                        break
                    self.sock.settimeout(1.0)
                    try:
                        data = self.sock.recv(1024).decode().strip()
                    except socket.timeout:
                        continue
                if data:
                    self.log(f"[DATA] {data}")
                    self.parse_data(data)
        except Exception as e:
            self.log(f"[!] 接收线程错误: {e}")
        finally:
            self.connection_status = False
            self.log("[!] 接收线程退出")
            with self.sock_lock:
                try:
                    if self.sock:
                        self.sock.close()
                except:
                    pass
                self.sock = None
            self.update_ui_state()

    def send_command(self, cmd):
        with self.sock_lock:
            if self.sock:
                try:
                    self.sock.send((cmd + "\r\n").encode())
                    self.log(f"[→] 发送: {cmd}")
                except Exception as e:
                    self.log(f"[!] 发送失败: {e}")
            else:
                self.log("[!] 尚未连接")

    def send_threshold(self):
        val = self.threshold_input.text().strip()
        if val.isdigit():
            self.send_command(f"#THRESHOLD={val}#")

    def parse_data(self, data):
        if not (data.startswith('#') and data.endswith('#')):
            return
        try:
            parts = data.strip('#').split('-')
            temp_full = parts[0].split('.')
            temp = int(temp_full[0]) + int(temp_full[1]) / 10
            humi = int(parts[1])
            light = int(parts[2])  # Used for plotting
            soil = int(parts[3])
            self.mode_flag = int(parts[4])  # 自动模式/手动模式
            self.pump_flag = int(parts[5])
            self.threshold = int(parts[6])
            light_lux = int(parts[7])  # Used for display

            self.data_display.setText(
                f"温度: {temp:.1f} ℃  湿度: {humi}%  光照: {light_lux} lx  土壤湿度: {soil}%"
            )
            self.status_display.setText(
                f"模式: {'自动模式' if self.mode_flag == 1 else '手动模式'}  "
                f"水泵: {'开启' if self.pump_flag == 1 else '关闭'}  "
                f"阈值: {self.threshold}"
            )

            now = time.time()
            now_str = time.strftime("%H:%M:%S", time.localtime(now))
            self.data_history['time'].append(now)
            self.data_history['time_str'].append(now_str)
            self.data_history['temp'].append(temp)
            self.data_history['humi'].append(humi)
            self.data_history['light'].append(light)
            self.data_history['soil'].append(soil)

            if len(self.data_history['time']) > 100:
                for key in self.data_history:
                    self.data_history[key] = self.data_history[key][-100:]

            self.update_ui_state()
        except Exception as e:
            self.log(f"[!] 数据解析错误: {e}")

    def save_data(self):
        try:
            filename, _ = QFileDialog.getSaveFileName(self, "保存数据", "", "CSV Files (*.csv)")
            if filename:
                df = pd.DataFrame(self.data_history)
                df.to_csv(filename, index=False)
                self.log(f"[+] 数据已保存到: {filename}")
        except Exception as e:
            self.log(f"[!] 保存数据失败: {e}")

    def load_data(self):
        try:
            filename, _ = QFileDialog.getOpenFileName(self, "加载数据", "", "CSV Files (*.csv)")
            if filename:
                df = pd.read_csv(filename)
                self.data_history = {
                    'temp': df['temp'].tolist(),
                    'humi': df['humi'].tolist(),
                    'light': df['light'].tolist(),
                    'soil': df['soil'].tolist(),
                    'time': df['time'].tolist(),
                    'time_str': df['time_str'].tolist()
                }
                self.log(f"[+] 已加载数据从: {filename}")
                self.update_plot()
        except Exception as e:
            self.log(f"[!] 加载数据失败: {e}")

    def update_plot(self):
        if not self.data_history['time']:
            return

        data_len = len(self.data_history['time'])
        x = list(range(data_len))
        time_labels = self.data_history['time_str']

        # === 动态调整时间间隔 ===
        if data_len <= 10:
            step = 1
        elif data_len <= 30:
            step = 2
        elif data_len <= 60:
            step = 5
        elif data_len <= 100:
            step = 10
        else:
            step = max(1, data_len // 10)

        ticks = [(i, time_labels[i]) for i in range(0, data_len, step)]

        ax = self.plot_widget.getAxis('bottom')
        ax.setTicks([ticks])

        self.temp_curve.setData(x, self.data_history['temp'])
        self.humi_curve.setData(x, self.data_history['humi'])
        self.light_curve.setData(x, self.data_history['light'])
        self.soil_curve.setData(x, self.data_history['soil'])

    def update_ui_state(self):
        connected = self.connection_status
        self.connect_button.setEnabled(not connected)
        self.disconnect_button.setEnabled(connected)
        self.send_threshold_button.setEnabled(connected)
        self.threshold_input.setEnabled(connected)
        self.save_data_button.setEnabled(bool(self.data_history['time']))  # Enable if data exists
        self.load_data_button.setEnabled(True)  # Always enabled

        if connected:
            self.pump_on_button.setEnabled(self.pump_flag == 0)
            self.pump_off_button.setEnabled(self.pump_flag == 1)
            self.auto_mode_button.setEnabled(self.mode_flag == 0)
            self.manual_mode_button.setEnabled(self.mode_flag == 1)
        else:
            self.pump_on_button.setEnabled(False)
            self.pump_off_button.setEnabled(False)
            self.auto_mode_button.setEnabled(False)
            self.manual_mode_button.setEnabled(False)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    # 设置全局字体
    app.setFont(QFont("微软雅黑", 12))
    window = IrrigationUI()
    window.show()
    sys.exit(app.exec_())