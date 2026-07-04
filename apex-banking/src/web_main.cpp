#include "Bank.h"
#include "Errors.h"
#include "Money.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;

std::string socketErrorText() {
    return "WinSock error " + std::to_string(WSAGetLastError());
}

void closeSocket(SocketHandle socket) {
    closesocket(socket);
}
#else
using SocketHandle = int;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;

std::string socketErrorText() {
    return std::strerror(errno);
}

void closeSocket(SocketHandle socket) {
    close(socket);
}
#endif

bool g_running = true;

void onSignal(int) {
    g_running = false;
}

std::string htmlPage() {
    return R"HTML(<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>APEX Banking Web Dashboard</title>
  <style>
    :root {
      --bg: #eef3fb;
      --card: #ffffff;
      --text: #152238;
      --muted: #667085;
      --line: #e4e7ec;
      --primary: #2563eb;
      --primary-dark: #1d4ed8;
      --ok: #16a34a;
      --bad: #dc2626;
      --warn: #f59e0b;
      --shadow: 0 18px 45px rgba(15, 23, 42, 0.08);
      font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: radial-gradient(circle at top left, #dbeafe 0, transparent 32rem), var(--bg);
      color: var(--text);
    }

    header {
      padding: 28px 32px 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 16px;
    }

    .brand h1 { margin: 0; font-size: 28px; letter-spacing: -0.04em; }
    .brand p { margin: 6px 0 0; color: var(--muted); }

    .shell {
      display: grid;
      grid-template-columns: 250px 1fr;
      gap: 22px;
      padding: 0 32px 32px;
    }

    nav {
      background: rgba(255, 255, 255, 0.78);
      backdrop-filter: blur(16px);
      border: 1px solid rgba(255, 255, 255, 0.72);
      border-radius: 24px;
      padding: 12px;
      box-shadow: var(--shadow);
      height: fit-content;
      position: sticky;
      top: 20px;
    }

    nav button {
      width: 100%;
      border: 0;
      border-radius: 16px;
      background: transparent;
      color: var(--text);
      padding: 13px 14px;
      text-align: left;
      font-size: 15px;
      cursor: pointer;
      margin: 3px 0;
    }

    nav button.active, nav button:hover {
      background: var(--primary);
      color: white;
    }

    main { min-width: 0; }
    .view { display: none; }
    .view.active { display: block; }

    .grid { display: grid; gap: 16px; }
    .stats { grid-template-columns: repeat(4, minmax(160px, 1fr)); }
    .two { grid-template-columns: 1.2fr 0.8fr; }

    .card {
      background: rgba(255, 255, 255, 0.9);
      border: 1px solid rgba(255, 255, 255, 0.78);
      border-radius: 24px;
      box-shadow: var(--shadow);
      padding: 20px;
      min-width: 0;
    }

    .metric-label { color: var(--muted); font-size: 13px; margin-bottom: 8px; }
    .metric-value { font-size: 32px; font-weight: 800; letter-spacing: -0.04em; }
    .card h2 { margin: 0 0 16px; font-size: 19px; }
    .card h3 { margin: 0 0 12px; font-size: 16px; }

    .toolbar {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      margin-bottom: 16px;
    }

    button.primary, button.soft {
      border: 0;
      border-radius: 14px;
      padding: 11px 14px;
      cursor: pointer;
      font-weight: 700;
    }

    button.primary { background: var(--primary); color: white; }
    button.primary:hover { background: var(--primary-dark); }
    button.soft { background: #edf2ff; color: var(--primary); }

    table {
      width: 100%;
      border-collapse: collapse;
      overflow: hidden;
      border-radius: 16px;
      font-size: 14px;
    }

    th, td {
      border-bottom: 1px solid var(--line);
      padding: 12px 10px;
      text-align: left;
      vertical-align: top;
    }

    th { color: var(--muted); font-size: 12px; text-transform: uppercase; letter-spacing: 0.06em; }
    tr:last-child td { border-bottom: 0; }

    .pill {
      display: inline-flex;
      align-items: center;
      border-radius: 999px;
      padding: 4px 9px;
      font-weight: 700;
      font-size: 12px;
    }

    .pill.ok { background: #dcfce7; color: #166534; }
    .pill.bad { background: #fee2e2; color: #991b1b; }
    .pill.type { background: #eef2ff; color: #3730a3; }

    .bar-row { margin: 13px 0; }
    .bar-head { display: flex; justify-content: space-between; gap: 12px; margin-bottom: 6px; color: var(--muted); font-size: 13px; }
    .bar-track { height: 12px; background: #e5e7eb; border-radius: 999px; overflow: hidden; }
    .bar-fill { height: 100%; width: 0%; background: linear-gradient(90deg, #60a5fa, #2563eb); border-radius: 999px; transition: width 240ms ease; }

    .donut-wrap { display: flex; align-items: center; gap: 18px; }
    .donut {
      width: 128px;
      height: 128px;
      border-radius: 50%;
      background: conic-gradient(var(--ok) 0deg, var(--ok) 0deg, var(--bad) 0deg 360deg);
      position: relative;
      flex: 0 0 auto;
    }
    .donut::after {
      content: "";
      position: absolute;
      inset: 22px;
      background: white;
      border-radius: 50%;
    }
    .legend { color: var(--muted); line-height: 1.9; }
    .dot { display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 8px; }
    .dot.ok { background: var(--ok); }
    .dot.bad { background: var(--bad); }

    dialog {
      border: 0;
      border-radius: 24px;
      width: min(520px, calc(100vw - 28px));
      padding: 0;
      box-shadow: 0 24px 80px rgba(15, 23, 42, 0.25);
    }
    dialog::backdrop { background: rgba(15, 23, 42, 0.36); }
    .modal { padding: 22px; }
    .modal h2 { margin: 0 0 14px; }
    label { display: block; margin: 12px 0 6px; color: var(--muted); font-size: 13px; font-weight: 700; }
    input, select {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 12px 13px;
      outline: 0;
      font-size: 15px;
    }
    input:focus, select:focus { border-color: var(--primary); box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.12); }
    .modal-actions { display: flex; justify-content: flex-end; gap: 10px; margin-top: 18px; }

    pre {
      white-space: pre-wrap;
      background: #0f172a;
      color: #e5e7eb;
      border-radius: 18px;
      padding: 16px;
      max-height: 320px;
      overflow: auto;
    }

    .toast {
      position: fixed;
      right: 24px;
      bottom: 24px;
      background: #0f172a;
      color: white;
      padding: 13px 16px;
      border-radius: 14px;
      box-shadow: var(--shadow);
      display: none;
      max-width: min(460px, calc(100vw - 48px));
      z-index: 20;
    }
    .toast.show { display: block; }

    @media (max-width: 980px) {
      .shell { grid-template-columns: 1fr; padding: 0 16px 24px; }
      header { padding: 22px 16px 14px; }
      nav { position: static; display: grid; grid-template-columns: repeat(2, 1fr); }
      .stats, .two { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <header>
    <div class="brand">
      <h1>APEX Banking Dashboard</h1>
      <p>Giao diện web local cho đồ án C++ ngân hàng đa tiền tệ.</p>
    </div>
    <button class="primary" onclick="loadState()">Refresh</button>
  </header>

  <div class="shell">
    <nav>
      <button class="active" data-view="dashboard">Dashboard</button>
      <button data-view="accounts">Accounts</button>
      <button data-view="transactions">Transactions</button>
      <button data-view="currency">Currency & Demo</button>
    </nav>

    <main>
      <section id="dashboard" class="view active">
        <div class="grid stats">
          <div class="card"><div class="metric-label">Tài khoản</div><div class="metric-value" id="mAccounts">0</div></div>
          <div class="card"><div class="metric-label">Giao dịch</div><div class="metric-value" id="mTx">0</div></div>
          <div class="card"><div class="metric-label">Thành công</div><div class="metric-value" id="mOk">0</div></div>
          <div class="card"><div class="metric-label">Thất bại</div><div class="metric-value" id="mBad">0</div></div>
        </div>
        <div class="grid two" style="margin-top:16px">
          <div class="card">
            <h2>Số dư theo tài khoản</h2>
            <div id="balanceBars"></div>
          </div>
          <div class="card">
            <h2>Tỷ lệ giao dịch</h2>
            <div class="donut-wrap">
              <div id="txDonut" class="donut"></div>
              <div class="legend">
                <div><span class="dot ok"></span><span id="okText">0 thành công</span></div>
                <div><span class="dot bad"></span><span id="badText">0 thất bại</span></div>
              </div>
            </div>
          </div>
        </div>
        <div class="card" style="margin-top:16px">
          <h2>Tổng số dư theo tiền tệ</h2>
          <div id="currencyTotals"></div>
        </div>
      </section>

      <section id="accounts" class="view">
        <div class="card">
          <h2>Quản lý tài khoản</h2>
          <div class="toolbar">
            <button class="primary" onclick="openModal('savings')">+ Savings</button>
            <button class="primary" onclick="openModal('checking')">+ Checking</button>
            <button class="soft" onclick="openModal('deposit')">Nạp tiền</button>
            <button class="soft" onclick="openModal('withdraw')">Rút tiền</button>
            <button class="soft" onclick="openModal('transfer')">Chuyển khoản</button>
          </div>
          <div style="overflow:auto"><table id="accountsTable"></table></div>
        </div>
      </section>

      <section id="transactions" class="view">
        <div class="card">
          <h2>Sổ cái giao dịch</h2>
          <div style="overflow:auto"><table id="txTable"></table></div>
        </div>
      </section>

      <section id="currency" class="view">
        <div class="grid two">
          <div class="card">
            <h2>Tỷ giá</h2>
            <div class="toolbar">
              <button class="primary" onclick="openModal('rate')">Cập nhật tỷ giá</button>
              <button class="soft" onclick="runInvalidRateDemo()">Demo invalid rate</button>
              <button class="soft" onclick="openModal('doubleSpend')">Demo double-spend</button>
            </div>
            <div style="overflow:auto"><table id="ratesTable"></table></div>
          </div>
          <div class="card">
            <h2>Log demo</h2>
            <pre id="demoLog">Chưa chạy demo.</pre>
          </div>
        </div>
      </section>
    </main>
  </div>

  <dialog id="modal"><form method="dialog" class="modal" id="modalForm"></form></dialog>
  <div id="toast" class="toast"></div>

  <script>
    const api = (path, data) => fetch(path, {
      method: data ? 'POST' : 'GET',
      headers: data ? {'Content-Type': 'application/x-www-form-urlencoded'} : {},
      body: data ? new URLSearchParams(data) : undefined
    }).then(r => r.json());

    let state = null;

    document.querySelectorAll('nav button').forEach(btn => {
      btn.addEventListener('click', () => {
        document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById(btn.dataset.view).classList.add('active');
      });
    });

    function money(n) {
      const x = Number(n || 0);
      return x.toLocaleString('vi-VN', {maximumFractionDigits: 2});
    }

    function toast(msg) {
      const el = document.getElementById('toast');
      el.textContent = msg;
      el.classList.add('show');
      setTimeout(() => el.classList.remove('show'), 2600);
    }

    function table(el, headers, rows) {
      el.innerHTML = '<thead><tr>' + headers.map(h => `<th>${h}</th>`).join('') + '</tr></thead>' +
        '<tbody>' + rows.map(row => '<tr>' + row.map(c => `<td>${c}</td>`).join('') + '</tr>').join('') + '</tbody>';
    }

    async function loadState() {
      state = await api('/api/state');
      render();
    }

    function render() {
      document.getElementById('mAccounts').textContent = state.summary.accounts;
      document.getElementById('mTx').textContent = state.summary.transactions;
      document.getElementById('mOk').textContent = state.summary.success;
      document.getElementById('mBad').textContent = state.summary.failed;

      renderBalanceBars();
      renderCurrencyTotals();
      renderDonut();
      renderAccounts();
      renderRates();
      renderTransactions();
    }

    function renderBalanceBars() {
      const wrap = document.getElementById('balanceBars');
      const max = Math.max(1, ...state.accounts.map(a => Math.abs(a.balance)));
      wrap.innerHTML = state.accounts.map(a => {
        const pct = Math.max(3, Math.min(100, Math.abs(a.balance) / max * 100));
        return `<div class="bar-row">
          <div class="bar-head"><b>${a.id} · ${a.owner}</b><span>${money(a.balance)} ${a.currency}</span></div>
          <div class="bar-track"><div class="bar-fill" style="width:${pct}%"></div></div>
        </div>`;
      }).join('') || '<p>Chưa có tài khoản.</p>';
    }

    function renderCurrencyTotals() {
      const wrap = document.getElementById('currencyTotals');
      const entries = Object.entries(state.summary.totalsByCurrency || {});
      const max = Math.max(1, ...entries.map(x => Math.abs(x[1])));
      wrap.innerHTML = entries.map(([cur, val]) => {
        const pct = Math.max(3, Math.min(100, Math.abs(val) / max * 100));
        return `<div class="bar-row">
          <div class="bar-head"><b>${cur}</b><span>${money(val)} ${cur}</span></div>
          <div class="bar-track"><div class="bar-fill" style="width:${pct}%"></div></div>
        </div>`;
      }).join('') || '<p>Chưa có dữ liệu.</p>';
    }

    function renderDonut() {
      const ok = state.summary.success;
      const bad = state.summary.failed;
      const total = Math.max(1, ok + bad);
      const deg = ok / total * 360;
      document.getElementById('txDonut').style.background = `conic-gradient(var(--ok) 0deg ${deg}deg, var(--bad) ${deg}deg 360deg)`;
      document.getElementById('okText').textContent = `${ok} thành công`;
      document.getElementById('badText').textContent = `${bad} thất bại`;
    }

    function renderAccounts() {
      table(document.getElementById('accountsTable'), ['ID', 'Chủ TK', 'Loại', 'Tiền tệ', 'Số dư'],
        state.accounts.map(a => [a.id, a.owner, `<span class="pill type">${a.kind}</span>`, a.currency, money(a.balance)]));
    }

    function renderRates() {
      table(document.getElementById('ratesTable'), ['Tiền tệ', 'Rate / USD'],
        state.rates.map(r => [r.code, money(r.rate)]));
    }

    function renderTransactions() {
      table(document.getElementById('txTable'), ['ID', 'Thời gian', 'Loại', 'Trạng thái', 'From', 'To', 'Amount', 'Ghi chú'],
        state.ledger.slice().reverse().map(tx => [
          tx.id,
          tx.time,
          tx.type,
          tx.status === 'SUCCESS' ? '<span class="pill ok">SUCCESS</span>' : '<span class="pill bad">FAILED</span>',
          tx.from || '-',
          tx.to || '-',
          `${money(tx.amount)} ${tx.currency}`,
          tx.note || '-'
        ]));
    }

    function field(name, label, type = 'text', value = '') {
      return `<label for="${name}">${label}</label><input id="${name}" name="${name}" type="${type}" value="${value}" required />`;
    }

    function openModal(kind) {
      const modal = document.getElementById('modal');
      const form = document.getElementById('modalForm');
      const titles = {
        savings: 'Tạo tài khoản Savings', checking: 'Tạo tài khoản Checking', deposit: 'Nạp tiền',
        withdraw: 'Rút tiền', transfer: 'Chuyển khoản', rate: 'Cập nhật tỷ giá', doubleSpend: 'Demo double-spend'
      };
      let body = `<h2>${titles[kind]}</h2>`;
      if (kind === 'savings') body += field('owner','Chủ tài khoản') + field('currency','Tiền tệ','text','USD') + field('amount','Số dư ban đầu','number','100') + field('interestRate','Lãi suất','number','0.05');
      if (kind === 'checking') body += field('owner','Chủ tài khoản') + field('currency','Tiền tệ','text','USD') + field('amount','Số dư ban đầu','number','100') + field('overdraftLimit','Hạn mức thấu chi','number','50');
      if (kind === 'deposit' || kind === 'withdraw') body += field('id','Mã tài khoản','text','acc_001') + field('amount','Số tiền','number','10') + field('currency','Tiền tệ','text','USD');
      if (kind === 'transfer') body += field('fromId','Từ tài khoản','text','acc_001') + field('toId','Đến tài khoản','text','acc_002') + field('amount','Số tiền','number','10') + field('currency','Tiền tệ','text','USD');
      if (kind === 'rate') body += field('code','Mã tiền tệ','text','EUR') + field('rate','Rate mới','number','0.92');
      if (kind === 'doubleSpend') body += field('id','Mã tài khoản','text','acc_001') + field('amount','Số tiền mỗi terminal rút','number','80');
      body += '<div class="modal-actions"><button class="soft" value="cancel">Hủy</button><button class="primary" value="default">Xác nhận</button></div>';
      form.innerHTML = body;
      form.onsubmit = async (e) => {
        e.preventDefault();
        const data = Object.fromEntries(new FormData(form).entries());
        try {
          const path = {
            savings: '/api/accounts/savings', checking: '/api/accounts/checking', deposit: '/api/deposit',
            withdraw: '/api/withdraw', transfer: '/api/transfer', rate: '/api/rates', doubleSpend: '/api/demo/double-spend'
          }[kind];
          const res = await api(path, data);
          if (!res.ok) throw new Error(res.message || 'Có lỗi xảy ra');
          if (res.log) document.getElementById('demoLog').textContent = res.log;
          toast(res.message || 'Thao tác thành công');
          modal.close();
          await loadState();
        } catch (err) {
          toast(err.message);
        }
      };
      modal.showModal();
    }

    async function runInvalidRateDemo() {
      const res = await api('/api/demo/invalid-rate', {});
      document.getElementById('demoLog').textContent = res.log || res.message;
      toast(res.message || 'Đã chạy demo');
      await loadState();
    }

    loadState();
  </script>
</body>
</html>)HTML";
}

std::string urlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());

    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') {
            out.push_back(' ');
        } else if (s[i] == '%' && i + 2 < s.size()) {
            std::string hex = s.substr(i + 1, 2);
            char* end = nullptr;
            long value = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                out.push_back(static_cast<char>(value));
                i += 2;
            }
        } else {
            out.push_back(s[i]);
        }
    }

    return out;
}

std::map<std::string, std::string> parseForm(const std::string& body) {
    std::map<std::string, std::string> result;
    std::size_t start = 0;

    while (start <= body.size()) {
        std::size_t end = body.find('&', start);
        if (end == std::string::npos) end = body.size();

        std::string pair = body.substr(start, end - start);
        std::size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            result[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        }

        if (end == body.size()) break;
        start = end + 1;
    }

    return result;
}

std::string jsonEscape(const std::string& s) {
    std::ostringstream os;
    for (char c : s) {
        switch (c) {
            case '"': os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    os << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    os << c;
                }
        }
    }
    return os.str();
}

std::string q(const std::string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

std::string timeText(std::time_t t) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

std::string txTypeName(TxType type) {
    switch (type) {
        case TxType::CREATE_ACCOUNT: return "CREATE_ACCOUNT";
        case TxType::DEPOSIT: return "DEPOSIT";
        case TxType::WITHDRAW: return "WITHDRAW";
        case TxType::TRANSFER: return "TRANSFER";
        case TxType::SET_RATE: return "SET_RATE";
        case TxType::APPLY_INTEREST: return "APPLY_INTEREST";
    }
    return "UNKNOWN";
}

std::string txStatusName(TxStatus status) {
    return status == TxStatus::SUCCESS ? "SUCCESS" : "FAILED";
}

std::string getRequired(const std::map<std::string, std::string>& form, const std::string& key) {
    auto it = form.find(key);
    if (it == form.end() || it->second.empty()) {
        throw std::runtime_error("Missing field: " + key);
    }
    return it->second;
}

double getDouble(const std::map<std::string, std::string>& form, const std::string& key) {
    std::string text = getRequired(form, key);
    std::size_t consumed = 0;
    double value = std::stod(text, &consumed);
    if (consumed != text.size()) {
        throw std::runtime_error("Invalid number: " + key);
    }
    return value;
}

std::string stateJson(const Bank& bank) {
    auto accounts = bank.allAccounts();
    const auto& ledger = bank.ledgerEntries();

    int success = 0;
    int failed = 0;
    for (const auto& tx : ledger) {
        if (tx.status() == TxStatus::SUCCESS) ++success;
        else ++failed;
    }

    std::map<std::string, double> totals;
    for (const Account* acc : accounts) {
        totals[acc->getCurrency()] += acc->getBalance();
    }

    std::ostringstream os;
    os << std::fixed << std::setprecision(6);
    os << "{";

    os << "\"summary\":{";
    os << "\"accounts\":" << accounts.size() << ",";
    os << "\"transactions\":" << ledger.size() << ",";
    os << "\"success\":" << success << ",";
    os << "\"failed\":" << failed << ",";
    os << "\"totalsByCurrency\":{";
    bool first = true;
    for (const auto& [cur, value] : totals) {
        if (!first) os << ",";
        first = false;
        os << q(cur) << ":" << value;
    }
    os << "}}";

    os << ",\"accounts\":[";
    first = true;
    for (const Account* acc : accounts) {
        if (!first) os << ",";
        first = false;
        os << "{";
        os << "\"id\":" << q(acc->getId()) << ",";
        os << "\"owner\":" << q(acc->getOwner()) << ",";
        os << "\"kind\":" << q(acc->kind()) << ",";
        os << "\"currency\":" << q(acc->getCurrency()) << ",";
        os << "\"balance\":" << acc->getBalance();
        os << "}";
    }
    os << "]";

    os << ",\"rates\":[";
    first = true;
    for (const auto& [code, rate] : bank.rates().allRates()) {
        if (!first) os << ",";
        first = false;
        os << "{\"code\":" << q(code) << ",\"rate\":" << rate << "}";
    }
    os << "]";

    os << ",\"ledger\":[";
    first = true;
    for (const auto& tx : ledger) {
        if (!first) os << ",";
        first = false;
        os << "{";
        os << "\"id\":" << tx.id() << ",";
        os << "\"type\":" << q(txTypeName(tx.type())) << ",";
        os << "\"status\":" << q(txStatusName(tx.status())) << ",";
        os << "\"from\":" << q(tx.fromId()) << ",";
        os << "\"to\":" << q(tx.toId()) << ",";
        os << "\"amount\":" << tx.amount().amount << ",";
        os << "\"currency\":" << q(tx.amount().currency) << ",";
        os << "\"note\":" << q(tx.note()) << ",";
        os << "\"time\":" << q(timeText(tx.timestamp()));
        os << "}";
    }
    os << "]";

    os << "}";
    return os.str();
}

std::string jsonOk(const std::string& message, const std::optional<std::string>& log = std::nullopt) {
    std::ostringstream os;
    os << "{\"ok\":true,\"message\":" << q(message);
    if (log.has_value()) {
        os << ",\"log\":" << q(*log);
    }
    os << "}";
    return os.str();
}

std::string jsonError(const std::string& message) {
    return "{\"ok\":false,\"message\":" + q(message) + "}";
}

void seed(Bank& bank) {
    bank.setRate("USD", 1.0);
    bank.setRate("VND", 24500.0);
    bank.setRate("EUR", 0.92);
    bank.setRate("JPY", 155.0);

    bank.createSavings("Nguyen Van A", "USD", Money{200.0, "USD"}, 0.05);
    bank.createChecking("Tran Thi B", "VND", Money{1000000.0, "VND"}, 500000.0);
}

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::optional<HttpRequest> readRequest(SocketHandle client) {
    std::string raw;
    char buffer[4096];

    while (raw.find("\r\n\r\n") == std::string::npos) {
        int n = recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (n <= 0) return std::nullopt;
        raw.append(buffer, static_cast<std::size_t>(n));
        if (raw.size() > 1024 * 1024) return std::nullopt;
    }

    std::size_t headerEnd = raw.find("\r\n\r\n");
    std::string headerPart = raw.substr(0, headerEnd);
    std::string body = raw.substr(headerEnd + 4);

    std::istringstream hs(headerPart);
    std::string requestLine;
    std::getline(hs, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();

    std::istringstream rl(requestLine);
    HttpRequest req;
    rl >> req.method >> req.path;

    std::string line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        req.headers[key] = value;
    }

    std::size_t contentLength = 0;
    auto it = req.headers.find("Content-Length");
    if (it != req.headers.end()) {
        contentLength = static_cast<std::size_t>(std::stoul(it->second));
    }

    while (body.size() < contentLength) {
        int n = recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (n <= 0) break;
        body.append(buffer, static_cast<std::size_t>(n));
    }

    if (body.size() > contentLength) body.resize(contentLength);
    req.body = std::move(body);
    return req;
}

void sendResponse(SocketHandle client, int status, const std::string& type, const std::string& body) {
    std::string statusText = status == 200 ? "OK" : status == 404 ? "Not Found" : "Bad Request";
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << statusText << "\r\n";
    response << "Content-Type: " << type << "; charset=utf-8\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << body;

    std::string text = response.str();
    send(client, text.data(), static_cast<int>(text.size()), 0);
}

std::pair<int, std::string> handleApi(Bank& bank, const HttpRequest& req) {
    try {
        if (req.method == "GET" && req.path == "/api/state") {
            return {200, stateJson(bank)};
        }

        if (req.method != "POST") {
            return {400, jsonError("Method not allowed")};
        }

        auto form = parseForm(req.body);

        if (req.path == "/api/accounts/savings") {
            std::string owner = getRequired(form, "owner");
            std::string currency = getRequired(form, "currency");
            double amount = getDouble(form, "amount");
            double interestRate = getDouble(form, "interestRate");
            Account& acc = bank.createSavings(owner, currency, Money{amount, currency}, interestRate);
            return {200, jsonOk("Da tao Savings " + acc.getId())};
        }

        if (req.path == "/api/accounts/checking") {
            std::string owner = getRequired(form, "owner");
            std::string currency = getRequired(form, "currency");
            double amount = getDouble(form, "amount");
            double overdraftLimit = getDouble(form, "overdraftLimit");
            Account& acc = bank.createChecking(owner, currency, Money{amount, currency}, overdraftLimit);
            return {200, jsonOk("Da tao Checking " + acc.getId())};
        }

        if (req.path == "/api/deposit") {
            std::string id = getRequired(form, "id");
            std::string currency = getRequired(form, "currency");
            double amount = getDouble(form, "amount");
            bank.deposit(id, Money{amount, currency});
            return {200, jsonOk("Da nap tien")};
        }

        if (req.path == "/api/withdraw") {
            std::string id = getRequired(form, "id");
            std::string currency = getRequired(form, "currency");
            double amount = getDouble(form, "amount");
            bank.withdraw(id, Money{amount, currency});
            return {200, jsonOk("Da rut tien")};
        }

        if (req.path == "/api/transfer") {
            std::string fromId = getRequired(form, "fromId");
            std::string toId = getRequired(form, "toId");
            std::string currency = getRequired(form, "currency");
            double amount = getDouble(form, "amount");
            bank.transfer(fromId, toId, Money{amount, currency});
            return {200, jsonOk("Da chuyen khoan")};
        }

        if (req.path == "/api/rates") {
            std::string code = getRequired(form, "code");
            double rate = getDouble(form, "rate");
            bank.setRate(code, rate);
            return {200, jsonOk("Da cap nhat ty gia")};
        }

        if (req.path == "/api/demo/invalid-rate") {
            std::ostringstream log;
            log << "Demo invalid rate\n";

            try {
                bank.setRate("EUR", -1.5);
                log << "EUR = -1.5: unexpected success\n";
            } catch (const BankError& e) {
                log << "EUR = -1.5: FAILED -> " << e.what() << "\n";
            }

            try {
                bank.setRate("JPY", 0.0);
                log << "JPY = 0: unexpected success\n";
            } catch (const BankError& e) {
                log << "JPY = 0: FAILED -> " << e.what() << "\n";
            }

            return {200, jsonOk("Da chay demo invalid rate", log.str())};
        }

        if (req.path == "/api/demo/double-spend") {
            std::string id = getRequired(form, "id");
            double amount = getDouble(form, "amount");
            std::ostringstream captured;
            auto* old = std::cout.rdbuf(captured.rdbuf());
            try {
                bank.simulateDoubleSpend(id, amount);
            } catch (...) {
                std::cout.rdbuf(old);
                throw;
            }
            std::cout.rdbuf(old);
            return {200, jsonOk("Da chay demo double-spend", captured.str())};
        }

        return {404, jsonError("Endpoint not found")};
    } catch (const BankError& e) {
        return {400, jsonError(e.what())};
    } catch (const std::exception& e) {
        return {400, jsonError(e.what())};
    }
}

void handleClient(SocketHandle client, Bank& bank) {
    auto reqOpt = readRequest(client);
    if (!reqOpt.has_value()) {
        closeSocket(client);
        return;
    }

    const HttpRequest& req = *reqOpt;

    if (req.method == "GET" && (req.path == "/" || req.path == "/index.html")) {
        sendResponse(client, 200, "text/html", htmlPage());
    } else if (req.path.rfind("/api/", 0) == 0) {
        auto [status, body] = handleApi(bank, req);
        sendResponse(client, status, "application/json", body);
    } else {
        sendResponse(client, 404, "application/json", jsonError("Not found"));
    }

    closeSocket(client);
}

} // namespace

int main(int argc, char** argv) {
    int port = 8080;
    if (argc >= 2) {
        port = std::stoi(argv[1]);
    }

    std::signal(SIGINT, onSignal);
#ifdef SIGTERM
    std::signal(SIGTERM, onSignal);
#endif

#ifdef _WIN32
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    Bank bank;
    seed(bank);

    SocketHandle server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET_HANDLE) {
        std::cerr << "socket failed: " << socketErrorText() << "\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int yes = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind failed: " << socketErrorText() << "\n";
        closeSocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(server, 16) < 0) {
        std::cerr << "listen failed: " << socketErrorText() << "\n";
        closeSocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "APEX Banking Web Dashboard\n";
    std::cout << "Open: http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n";

    while (g_running) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int len = sizeof(clientAddr);
#else
        socklen_t len = sizeof(clientAddr);
#endif
        SocketHandle client = accept(server, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (client == INVALID_SOCKET_HANDLE) {
#ifndef _WIN32
            if (errno == EINTR) continue;
#endif
            std::cerr << "accept failed: " << socketErrorText() << "\n";
            break;
        }
        handleClient(client, bank);
    }

    closeSocket(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
