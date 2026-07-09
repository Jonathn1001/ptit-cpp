# 🎬 Kịch bản Demo Video — APEX (Hệ thống ngân hàng đa tiền tệ)

> Đề 2 — OOP C++ (PTIT). Kịch bản quay video ~8–10 phút, trình diễn **đủ mọi
> yêu cầu của đề bài** theo thứ tự, kèm lời thoại và thao tác chính xác.
>
> Phiên bản CLI có **phân quyền Admin / User** (đăng nhập bằng Account ID).
> Toàn bộ output dưới đây được **chạy thật** và sao lại nguyên văn.

---

## 0. Chuẩn bị (trước khi quay)

```bash
cd apex-banking
make clean && make        # biên dịch, phải sạch 0 warning
make test                 # 35/35 passed — quay luôn màn này để chứng minh có unit test
./apex                    # khởi động chương trình
```

Khi chạy `./apex`, chương trình tự nạp **dữ liệu mẫu (seed)**:

```
[seed] Admin vào trực tiếp.
[seed] User demo đăng nhập bằng Account ID: acc_001, acc_002
[seed] Đã seed 4 currency và 2 account mẫu.
```

| Loại tiền | Tỷ giá (trên 1 USD) |
|---|---|
| USD | 1.0 |
| VND | 24 500 |
| EUR | 0.92 |
| JPY | 155 |

| Tài khoản | Loại | Chủ | Số dư | Đặc thù | Đăng nhập bằng |
|---|---|---|---|---|---|
| `acc_001` | Savings | Nguyễn Văn A | 200 USD | lãi 5% | `acc_001` |
| `acc_002` | Checking | Trần Thị B | 1 000 000 VND | thấu chi 500 000 | `acc_002` |

**Menu vai trò (màn hình đầu tiên):**
```
==============================================
 APEX Banking CLI
==============================================
 1. Admin
 2. User
 0. Thoát
```
→ Chọn `1` rồi `1` (Vào admin) để vào **ADMIN MENU** (làm được mọi thao tác).
→ Chọn `2` để vào luồng **User** (đăng nhập bằng Account ID, chỉ thấy tài khoản của mình).

---

## 🎯 Bảng ánh xạ: Yêu cầu đề bài → Cảnh demo

| Yêu cầu trong đề | Cảnh demo | Thao tác (trong ADMIN MENU) |
|---|---|---|
| Lớp `Account` cơ sở + `Savings`/`Checking` dẫn xuất | Cảnh 1 | 1, 2 |
| Dùng `std::map` lưu ID → đối tượng Tài khoản | Cảnh 2 | 3 |
| Nạp chồng toán tử `+` / `-` xử lý giao dịch + quy đổi tiền tệ | Cảnh 3 | 5, 6, 7 |
| Lớp `Transaction` ghi lại **mọi** hành động | Cảnh 4 | 10 |
| ⚠ Lỗi bắt buộc: **Chi tiêu kép (Double-Spend)** | Cảnh 5 | 12 |
| ⚠ Lỗi bắt buộc: **Tỷ giá = 0 hoặc âm** | Cảnh 6 | 13 |
| (Bonus) Phân quyền Admin/User + lãi suất + chống lỗi | Cảnh 7 | 11, luồng User |

> Thoại mở đầu: *"Chào thầy/cô, em xin trình bày đồ án Đề 2 — hệ thống ngân
> hàng đa tiền tệ Apex. Chương trình viết bằng C++20, kiến trúc hướng đối
> tượng, có phân quyền Admin/User, và em sẽ lần lượt chứng minh từng yêu cầu."*

---

## Cảnh 1 — Kế thừa OOP: `Account` → `Savings` / `Checking`

**Thoại:** *"Đề yêu cầu một lớp Account cơ sở với hai lớp con Savings và
Checking. Em vào bằng quyền Admin và tạo mới cả hai loại."*

Từ menu vai trò: chọn `1` (Admin) → `1` (Vào admin).

**Tạo tài khoản Savings** — chọn `1`:
```
Owner: Nguyen Van C
Currency (USD/VND/EUR/JPY): USD
Opening balance: 500
Interest rate (0.05 = 5%): 0.03
```
→ Kết quả:
```
Tạo Savings thành công. Account ID = acc_003
[Savings] acc_003  owner=Nguyen Van C  balance=500.00 USD  interest=3%
```

**Tạo tài khoản Checking** — chọn `2`:
```
Owner: Le Thi D
Currency (USD/VND/EUR/JPY): EUR
Opening balance: 300
Overdraft limit: 100
```
→ Kết quả:
```
Tạo Checking thành công. Account ID = acc_004
[Checking] acc_004  owner=Le Thi D  balance=300.00 EUR  overdraft=100.00
```

**Điểm cần nói rõ trước camera:**
- Cả hai đều **kế thừa** từ lớp trừu tượng `Account` (`Account.h`).
- Mỗi lớp con **override** hai hàm ảo `kind()` và `canWithdraw()`.
- `Savings` — có lãi suất, **không** cho rút quá số dư.
- `Checking` — không lãi, **cho** rút âm tới hạn mức thấu chi.
- → Đây là **đa hình (polymorphism)**: cùng lệnh `print()`, mỗi loại in ra khác nhau
  (dòng `interest=...%` vs `overdraft=...`).

---

## Cảnh 2 — `std::map` lưu ID → Tài khoản

**Thoại:** *"Đề yêu cầu dùng std::map để lưu ID tài khoản. Em dùng
`std::map<std::string, std::unique_ptr<Account>>` trong lớp Bank."*

Chọn `3` (Danh sách account):
```
[Savings] acc_001  owner=Nguyễn Văn A  balance=200.00 USD  interest=5%
[Checking] acc_002  owner=Trần Thị B  balance=1000000.00 VND  overdraft=500000.00
[Savings] acc_003  owner=Nguyen Van C  balance=500.00 USD  interest=3%
[Checking] acc_004  owner=Le Thi D  balance=300.00 EUR  overdraft=100.00
```

**Điểm cần nói rõ:**
- Danh sách in ra **tự động sắp xếp theo ID** (`acc_001 → acc_004`) — đúng
  bản chất `std::map` là cây cân bằng, luôn giữ khoá có thứ tự.
- Dùng `unique_ptr` → **sở hữu duy nhất**, tự giải phóng bộ nhớ, không rò rỉ.
- Mở file `Bank.h` chỉ vào dòng `std::map<...> accounts;` để chứng minh.

---

## Cảnh 3 — Nạp chồng toán tử `+` / `-` + Tự động quy đổi tiền tệ

**Thoại:** *"Đề yêu cầu nạp chồng toán tử + và - để xử lý giao dịch. Em cài
`operator+` cho nạp tiền, `operator-` cho rút tiền, và cả hai tự động quy đổi."*

**a) Nạp tiền CÙNG loại tiền** — chọn `5`:
```
Account ID: acc_001
Amount: 50
Currency: USD
```
→ `Nạp tiền thành công.` → `balance=250.00 USD`

**b) Nạp tiền KHÁC loại tiền (chứng minh quy đổi)** — chọn `5`:
```
Account ID: acc_001
Amount: 24500
Currency: VND
```
→ `balance=251.00 USD` — *"24 500 VND được tự động quy đổi = 1 USD nhờ `operator+`
gọi CurrencyRegistry."*

**c) Rút tiền** — chọn `6`:
```
Account ID: acc_001
Amount: 30
Currency: USD
```
→ `Rút tiền thành công.` → `balance=221.00 USD`

**d) Chuyển khoản LIÊN TIỀN TỆ (USD → tài khoản VND)** — chọn `7`:
```
From Account ID: acc_001
To Account ID: acc_002
Amount: 10
Currency: USD
```
→ `Chuyển khoản thành công.` — acc_001 giảm 10 USD (còn `211.00 USD`),
acc_002 tăng 245 000 VND (10 × 24 500).

**Điểm cần nói rõ:**
- `account + Money` = nạp; `account - Money` = rút (xem `Account.cpp`).
- `operator-` gọi `canWithdraw()` (đa hình) để kiểm tra chính sách từng loại tài khoản.
- Quy đổi đi qua USD làm gốc: `số tiền / tỷ_giá_nguồn × tỷ_giá_đích`.
- **Chuyển khoản là nguyên tử**: nếu bước cộng vào bên nhận lỗi, bước trừ bên gửi
  được **hoàn tác** — không bao giờ "bốc hơi" tiền (xem `Bank::transfer`).

---

## Cảnh 4 — Lớp `Transaction`: Sổ cái ghi MỌI hành động

**Thoại:** *"Đề yêu cầu một lớp Giao dịch ghi lại mọi hành động. Em có lớp
Transaction bất biến, và Bank ghi sổ cái cho cả giao dịch THÀNH CÔNG lẫn THẤT BẠI."*

Trước đó bấm `11 → acc_001` (áp dụng lãi) để có thêm dòng `APPLY_INTEREST`, rồi
chọn `10` (Ledger):
```
#0001  2026-07-07 21:50:07  SET_RATE       SUCCESS  amt=1.00 USD
#0002  2026-07-07 21:50:07  SET_RATE       SUCCESS  amt=24500.00 VND
#0003  2026-07-07 21:50:07  SET_RATE       SUCCESS  amt=0.92 EUR
#0004  2026-07-07 21:50:07  SET_RATE       SUCCESS  amt=155.00 JPY
#0005  2026-07-07 21:50:07  CREATE_ACCOUNT SUCCESS  to=acc_001  amt=200.00 USD  (Savings/Nguyễn Văn A)
#0006  2026-07-07 21:50:07  CREATE_ACCOUNT SUCCESS  to=acc_002  amt=1000000.00 VND  (Checking/Trần Thị B)
#0009  2026-07-07 21:50:07  DEPOSIT        SUCCESS  to=acc_001  amt=50.00 USD
#0010  2026-07-07 21:50:07  DEPOSIT        SUCCESS  to=acc_001  amt=24500.00 VND
#0011  2026-07-07 21:50:07  WITHDRAW       SUCCESS  from=acc_001  amt=30.00 USD
#0012  2026-07-07 21:50:07  TRANSFER       SUCCESS  from=acc_001  to=acc_002  amt=10.00 USD
#0013  2026-07-07 21:50:07  APPLY_INTEREST SUCCESS  to=acc_001  amt=221.55 USD
```

**Điểm cần nói rõ:**
- Mỗi dòng: mã tự tăng, **thời gian thực**, loại giao dịch, trạng thái, số tiền, ghi chú.
- 6 loại giao dịch: `CREATE_ACCOUNT / DEPOSIT / WITHDRAW / TRANSFER / SET_RATE / APPLY_INTEREST`.
- Dòng `#0013 APPLY_INTEREST` chứng minh **áp dụng lãi cũng được ghi sổ**.
- Quan trọng: **giao dịch thất bại vẫn được ghi** (`FAILED`) — sẽ thấy rõ ở Cảnh 5 & 6.

---

## ⚠ Cảnh 5 — LỖI BẮT BUỘC #1: Chi tiêu kép (Double-Spend)

**Thoại:** *"Đề yêu cầu minh hoạ lỗi Chi tiêu kép — hai luồng cùng rút tiền một
lúc. Em dùng `std::thread` thật, và `BankService` bảo vệ bằng `std::mutex` nên
hai giao dịch bị buộc chạy tuần tự — không thể chi tiêu kép."*

Chọn `12`:
```
Account ID demo: acc_001
Amount mỗi thread rút: 150
```

Output (số dư acc_001 lúc này = 221.55 USD):
```
--- Double-Spend demo: std::thread + std::mutex ---
Hai thread cùng rút 150 USD từ acc_001 cùng lúc.
BankService dùng std::mutex nên giao dịch chạy tuần tự.
Thread A: THÀNH CÔNG
Thread B: THẤT BẠI -> insufficient funds in acc_001
Account cuối: [Savings] acc_001  owner=Nguyễn Văn A  balance=71.55 USD  interest=5%
--- Kết thúc demo ---
```

**Điểm cần nói rõ:**
- Chọn số tiền **> một nửa số dư** (150 > 221.55/2) để chỉ **một** thread rút được
  → chứng minh rõ việc chống chi tiêu kép.
- Nếu KHÔNG có khoá: hai thread cùng đọc số dư cũ, cả hai cùng rút → mất tiền.
- Cách sửa: `BankService` bọc mỗi thao tác trong `std::lock_guard<std::mutex>`
  → giao dịch tuần tự, thread thứ hai thấy số dư đã giảm và bị `InsufficientFunds`.
- Số dư cuối `71.55` = `221.55 − 150` (đúng **một** lần rút, không phải hai).

---

## ⚠ Cảnh 6 — LỖI BẮT BUỘC #2: Tỷ giá bằng 0 hoặc âm

**Thoại:** *"Đề yêu cầu xử lý khi tỷ giá đặt bằng 0 hoặc âm. Hệ thống phải từ
chối và ghi lại. Thao tác này chỉ Admin mới được phép."*

Chọn `13` (demo tự động):
```
Test rate EUR = -1.5 ...
THẤT BẠI: rate must be > 0 (got -1.500000)
Test rate JPY = 0 ...
THẤT BẠI: rate must be > 0 (got 0.000000)
Bank vẫn ghi SET_RATE thất bại vào ledger.
```

(Tuỳ chọn) Chứng minh thủ công qua menu `9` (Cập nhật tỷ giá):
```
Currency code: USD
Rate mới (theo 1 USD): -5
```
→ `Lỗi: rate must be > 0 (got -5.000000)`

**Điểm cần nói rõ:**
- `CurrencyRegistry::setRate()` ném exception `InvalidRate` khi `rate <= 0`.
- Nhờ vậy **không bao giờ có tỷ giá âm/0** lọt vào hệ thống → mọi phép quy đổi luôn hợp lệ.
- Bấm lại menu `10` → thấy hai dòng `SET_RATE FAILED` được lưu.
- Cập nhật tỷ giá là **quyền Admin** — user thường không có menu này (xem Cảnh 7).

---

## Cảnh 7 (bonus) — Phân quyền Admin/User, lãi suất & chống lỗi

**a) Đăng nhập User và giới hạn quyền.** Về menu vai trò, chọn `2` (User) → `1`
(Nhập Account ID) → nhập `acc_001`:
```
Account ID: acc_001
Đã vào Account ID: acc_001
```
Trong **USER MENU** (của Nguyễn Văn A):
- `1` — xem **tài khoản của mình**: `[Savings] acc_001 ... balance=200.00 USD`.
- `5` — Rút tiền (tự dùng `acc_001`): `Amount: 20`, `Currency: USD` → `balance=180.00 USD`.
- `6` — Chuyển khoản: `To Account ID: acc_002`, `Amount: 5`, `Currency: USD`.
- `8` — **Ledger của tôi** — chỉ hiện giao dịch của chính user:
```
#0005  ...  CREATE_ACCOUNT SUCCESS  to=acc_001  amt=200.00 USD  (Savings/Nguyễn Văn A)
#0007  ...  WITHDRAW       SUCCESS  from=acc_001  amt=20.00 USD
#0008  ...  TRANSFER       SUCCESS  from=acc_001  to=acc_002  amt=5.00 USD
```

**Điểm cần nói rõ:**
- User **không** có menu cập nhật tỷ giá / áp lãi / xem toàn bộ account — đó là quyền Admin.
- `Ledger của tôi` **lọc theo quyền sở hữu**: user chỉ thấy giao dịch liên quan tài khoản
  của mình, không thấy của người khác (RBAC ở tầng đọc).
- Kiến trúc: `AuthService` (User + Role), `BankService` (kiểm tra `requireAdmin` /
  `requireOwnerOrAdmin` trước mỗi thao tác).

**b) Khoe khả năng chống lỗi đầu vào** (nói nhanh, gõ vài lệnh sai):
- Rút quá số dư → `Lỗi: insufficient funds in ...`.
- Nhập ID không tồn tại → `Lỗi: no such account: ...`.
- Tạo tài khoản với lãi/thấu chi âm → `Lỗi: ... must be >= 0`.
- Tạo tài khoản với tiền tệ chưa đăng ký (vd `ZZZ`) → `Lỗi: unknown currency: ZZZ`.
- Nhập chữ vào ô số (vd `100abc`) → `Lỗi: ký tự thừa sau số: 100abc`.

→ *"Toàn bộ lỗi được bắt tại một điểm duy nhất trong mỗi vòng lặp menu, chương
trình không bao giờ crash."*

---

## Kết (thoại đóng)

*"Tóm lại, đồ án đáp ứng đủ mọi yêu cầu của Đề 2:*
- *Kế thừa Account → Savings/Checking, có đa hình;*
- *Nạp chồng toán tử + và - kèm tự động quy đổi tiền tệ;*
- *std::map lưu ID tài khoản;*
- *Lớp Transaction ghi lại mọi hành động, kể cả thất bại;*
- *Trình diễn đủ hai tình huống lỗi bắt buộc: Chi tiêu kép và Tỷ giá không hợp lệ.*

*Bổ sung: phân quyền Admin/User (AuthService + BankService), chuyển khoản nguyên tử,
và 35 unit test tự động (doctest) đều pass. Em xin cảm ơn thầy/cô."*

---

### 📋 Chuỗi phím tắt — quay 1 mạch luồng ADMIN (đủ 6 yêu cầu)

```
1                                  # Menu vai trò: Admin
1                                  # Vào admin
1 → Nguyen Van C → USD → 500 → 0.03    # tạo Savings   (Cảnh 1)
2 → Le Thi D → EUR → 300 → 100         # tạo Checking  (Cảnh 1)
3                                      # list, chứng minh std::map (Cảnh 2)
5 → acc_001 → 50 → USD                 # nạp cùng tiền (Cảnh 3a)
5 → acc_001 → 24500 → VND              # nạp quy đổi   (Cảnh 3b)
6 → acc_001 → 30 → USD                 # rút           (Cảnh 3c)
7 → acc_001 → acc_002 → 10 → USD       # chuyển liên tiền tệ (Cảnh 3d)
11 → acc_001                           # áp dụng lãi (được ghi sổ) (Cảnh 4/7)
10                                     # in sổ cái     (Cảnh 4)
12 → acc_001 → 150                     # DOUBLE-SPEND  (Cảnh 5) ⚠
13                                     # TỶ GIÁ SAI    (Cảnh 6) ⚠
10                                     # in sổ cái lần cuối, thấy các dòng FAILED
0                                      # đăng xuất admin
0                                      # quay lại menu vai trò
0                                      # thoát
```

### 📋 Chuỗi phím tắt — luồng USER (bonus phân quyền, Cảnh 7)

```
2                                  # Menu vai trò: User
1                                  # Nhập Account ID
acc_001                            # đăng nhập (Nguyễn Văn A)
1                                  # xem tài khoản của tôi
5 → 20 → USD                       # rút (tự dùng acc_001)
6 → acc_002 → 5 → USD              # chuyển khoản
8                                  # Ledger của tôi (chỉ giao dịch của mình)
0                                  # đăng xuất user
0                                  # quay lại menu vai trò
0                                  # thoát
```
