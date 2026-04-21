# APEX — Hệ thống ngân hàng đa tiền tệ

> Đồ án giữa kỳ C++ (PTIT) — Đề 2: Multi-currency banking system.

Chương trình CLI tương tác mô phỏng một hệ thống ngân hàng chuyên nghiệp
quản lý nhiều loại tài khoản (Savings, Checking), hỗ trợ chuyển khoản
giữa các loại tiền tệ khác nhau, ghi sổ cái mọi giao dịch, và trình diễn
trực tiếp các tình huống lỗi theo yêu cầu đề bài.

## ✨ Tính năng

- Kế thừa: lớp `Account` trừu tượng → `Savings` (có lãi suất, không thấu chi)
  và `Checking` (không lãi suất, có hạn mức thấu chi).
- Nạp chồng toán tử `+` và `-` trên `Account` để nạp / rút tiền, **tự động
  quy đổi tiền tệ** qua `CurrencyRegistry`.
- `Bank` lưu trữ tài khoản trong `std::map<std::string, std::unique_ptr<Account>>`.
- Lớp `Transaction` ghi lại mọi giao dịch (kể cả thất bại) trong sổ cái.
- Demo tương tác hai tình huống lỗi theo yêu cầu:
  - Chi tiêu kép (Double-Spend) — minh hoạ cả phiên bản có lỗi và đã sửa.
  - Tỷ giá không hợp lệ — từ chối rate ≤ 0 và ghi vào sổ cái.

## 🛠 Yêu cầu hệ thống

- Trình biên dịch hỗ trợ **C++20** (`g++ ≥ 10` hoặc `clang++ ≥ 12`).
- **GNU Make**.
- Hệ điều hành: Linux / macOS / WSL / Windows (MinGW).

## 📦 Cài đặt & Biên dịch

```bash
git clone https://github.com/<user>/ptit-cpp.git
cd ptit-cpp/apex-banking
make
```

Kết quả: file thực thi `./apex`.

Để chạy bộ test:
```bash
make test
```

Dọn sạch:
```bash
make clean
```

## ▶️ Chạy chương trình

```bash
./apex
```

Chương trình khởi động menu tương tác với dữ liệu mẫu (4 loại tiền tệ
USD/VND/EUR/JPY và 2 tài khoản mẫu `acc_001`, `acc_002`).

## 🎬 Demo các tình huống lỗi

### 1. Double-Spend (Chi tiêu kép) — menu **12**

```
Phase 1 (BUGGY): Terminal A và B đều đọc balance = 100 trước khi ghi
                 → cả hai tin rằng còn đủ tiền, cả hai đều rút 80
                 → tài khoản bị trừ sai
Phase 2 (FIXED): Dùng Bank::lock() / unlock() để nối tiếp thao tác
                 → Terminal B bị từ chối với DoubleSpendDetected
```

### 2. Tỷ giá không hợp lệ — menu **13** (hoặc **9** với rate ≤ 0)

```
Thử đặt tỷ giá EUR = -1.5  →  ❌ InvalidRate: rate must be > 0
Thử đặt tỷ giá JPY = 0     →  ❌ InvalidRate: rate must be > 0
(Cả hai giao dịch FAILED được ghi vào sổ cái.)
```

## 🏗 Kiến trúc

```
┌─────────────────┐   owns   ┌──────────────────────────────────────┐
│     Bank        │ ───────▶ │ std::map<id, unique_ptr<Account>>    │
│ (facade)        │          └──────────────────────────────────────┘
│                 │ ──▶ CurrencyRegistry (tỷ giá)
│                 │ ──▶ std::vector<Transaction> (sổ cái)
└─────────────────┘
          ▲
          │ dùng
          │
     ┌────┴────┐
     │ main.cpp │   (menu CLI — không chứa logic nghiệp vụ)
     └─────────┘

    Account (abstract)   — có operator+, operator-
       ▲         ▲
       │         │
    Savings   Checking
```

## 📚 Nạp chồng toán tử

| Toán tử | Ngữ nghĩa |
|---|---|
| `account + Money` | Nạp tiền (tự động quy đổi tiền tệ) |
| `account - Money` | Rút tiền (tự động quy đổi + kiểm tra policy của loại tài khoản) |

## 📂 Cấu trúc thư mục

```
apex-banking/
├── include/       # Header files (.h)
├── src/           # Implementation (.cpp)
├── test/          # Unit tests (doctest)
├── Makefile
└── README.md
```

## 🧪 Testing

Dự án dùng [doctest](https://github.com/doctest/doctest) — thư viện test
header-only, được vendored sẵn trong `test/doctest.h`, không cần cài gì thêm.

```bash
make test
# → 30 passed | 0 failed
```

## 👤 Tác giả

PTIT — 2026.
