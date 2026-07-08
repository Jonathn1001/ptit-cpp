# APEX — Hệ thống ngân hàng đa tiền tệ

> Đồ án giữa kỳ C++ (PTIT) — Đề 2: Multi-currency banking system.

Chương trình CLI tương tác mô phỏng một hệ thống ngân hàng đa tiền tệ, quản lý nhiều loại account như `Savings` và `Checking`, hỗ trợ nạp tiền, rút tiền, chuyển khoản, quy đổi currency, ghi ledger và minh họa tình huống double-spend.

Bản này đã được chỉnh để logic chặt hơn nhưng vẫn giữ tối đa bố cục gốc: `include/`, `src/`, `test/`, `Makefile`, `README.md`.

## Tính năng chính

- Kế thừa: lớp `Account` trừu tượng, hai lớp con `Savings` và `Checking`.
- `Savings`: có interest rate, không cho overdraft.
- `Checking`: không có interest, có overdraft limit.
- Nạp chồng toán tử `+` và `-` trên `Account` để nạp/rút tiền.
- `CurrencyRegistry` quản lý tỷ giá theo 1 USD và tự động quy đổi giữa nhiều currency.
- `Bank` lưu account bằng `std::map<std::string, std::unique_ptr<Account>>`.
- `Transaction` ghi ledger cho các nghiệp vụ chính, bao gồm nhiều trường hợp thành công/thất bại.
- `BankService` kiểm soát quyền `admin`/`user` và dùng `std::mutex` để serialize giao dịch.
- Demo double-spend bằng thread thật qua menu 12.
- Demo tỷ giá lỗi qua menu 13.

## Yêu cầu hệ thống

- Trình biên dịch hỗ trợ C++20, ví dụ `g++ >= 10` hoặc `clang++ >= 12`.
- GNU Make.
- Linux / macOS / WSL / Windows MinGW.

## Cài đặt & biên dịch

```bash
git clone https://github.com/Jonathn1001/ptit-cpp.git
cd ptit-cpp/apex-banking
make
```

Kết quả: file thực thi `./apex`.

Chạy chương trình:

```bash
make run
```

Chạy test:

```bash
make test
```

Dọn file build:

```bash
make clean
```

## Dữ liệu mẫu khi khởi động

Chương trình seed sẵn:

- Currency: `USD`, `VND`, `EUR`, `JPY`.
- Account mẫu:
  - `acc_001`: `Savings`, owner `Nguyễn Văn A`, balance 200 USD, interest rate 0.05.
  - `acc_002`: `Checking`, owner `Trần Thị B`, balance 1,000,000 VND, overdraft limit 500,000 VND.
- `admin`: vào trực tiếp bằng menu `Admin`.
- `user`: đăng nhập bằng Account ID như `acc_001` hoặc `acc_002`.

Lưu ý: phần auth trong đồ án là mô phỏng CLI, chưa triển khai password hashing, session token hoặc database user thật.

## Menu chính

```text
1. Admin
2. User
3. Tạo user mới
0. Thoát
```

Menu trong phiên `admin`/`user`:

```text
-- Account --
1. Savings
2. Checking
3. Danh sách account
4. Chi tiết account

-- Giao dịch --
5. Nạp tiền
6. Rút tiền
7. Chuyển khoản

-- Currency --
8. Tỷ giá
9. Cập nhật tỷ giá       (admin)

-- Report & Demo --
10. Ledger
11. Lãi Savings
12. Demo double-spend
13. Demo tỷ giá lỗi      (admin)
14. Danh sách user       (admin)
0. Đăng xuất
```

## Demo double-spend — menu 12

Ví dụ với `acc_001` có 200 USD, nhập mỗi thread rút 150 USD:

```text
Terminal A: SUCCESS
Terminal B: FAILED (không đủ số dư hoặc vượt hạn mức)
```

Hoặc ngược lại tùy thread nào chạy trước. Điểm quan trọng là hai giao dịch không cùng đọc/sửa balance một lúc vì `BankService` có `std::mutex`. Giao dịch vào sau phải kiểm tra lại balance sau giao dịch trước.

## Demo tỷ giá lỗi — menu 13

Chương trình thử:

```text
EUR = -1.5
JPY = 0
```

Cả hai đều bị từ chối vì rate phải hữu hạn và > 0. Giao dịch lỗi được ghi vào ledger ở nghiệp vụ cập nhật tỷ giá.

## Kiến trúc

```text
main.cpp
   |
   v
BankService  -- kiểm soát quyền admin/user, mutex chống race
   |
   v
Bank  -- quản lý account, ledger, currency registry
   |
   +--> CurrencyRegistry
   +--> std::map<id, unique_ptr<Account>>
   +--> std::vector<Transaction>

Account (abstract)
   |
   +--> Savings
   +--> Checking
```

## Nạp chồng toán tử

| Toán tử | Ý nghĩa |
|---|---|
| `account + Money` | Nạp tiền, tự động quy đổi currency |
| `account - Money` | Rút tiền, tự động quy đổi và kiểm tra policy của account |

## Cấu trúc thư mục

```text
apex-banking/
├── include/       # Header files (.h)
├── src/           # Implementation (.cpp)
├── test/          # Test bằng assert
├── Makefile
├── .gitignore
└── README.md
```

## Một vài case test nhanh bằng menu

### Chặn amount = 0

```text
Admin -> 5. Nạp tiền
Account ID: acc_001
Số tiền: 0
Currency: USD
```

Kết quả: báo lỗi `số tiền giao dịch phải > 0`.

### Ledger interest đúng

```text
Admin -> 11. Lãi Savings
Savings Account ID: acc_001
Admin -> 10. Ledger
```

Với acc_001 ban đầu 200 USD, interest 5%, ledger ghi `10 USD` cho `APPLY_INTEREST`.

### Admin tạo account và gán cho User

```text
Admin -> 1. Savings
Owner: Nguyễn Văn C
Currency gốc: USD
Opening balance: 100
Interest rate: 0.05
Gán account này cho một User đăng nhập? y
User ID: Enter để dùng Account ID mới
```

Sau đó có thể đăng xuất, chọn `User`, đăng nhập bằng Account ID mới.

## Tác giả

PTIT — 2026.
