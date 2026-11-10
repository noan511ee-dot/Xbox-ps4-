<p align="center">
  <img src="assets/nawaf_logo.png" alt="PBOX logo" width="180">
</p>

<h1 align="center">PBOX</h1>
<p align="center"><strong>Cloud gaming client for PlayStation 4</strong></p>
<p align="center">A clean, community-built interface for streaming Xbox Cloud Gaming on a homebrew-enabled PS4.</p>

<p align="center">
  <a href="https://github.com/noan511ee-dot/Xbox/releases/latest"><img alt="latest release" src="https://img.shields.io/github/v/release/noan511ee-dot/Xbox?label=latest%20release&color=20c997"></a>
  <a href="https://github.com/noan511ee-dot/Xbox/actions/workflows/build-ps4-pkg.yml"><img alt="build" src="https://img.shields.io/github/actions/workflow/status/noan511ee-dot/Xbox/build-ps4-pkg.yml?label=PS4%20build&color=20c997"></a>
  <img alt="platform" src="https://img.shields.io/badge/platform-PS4-003791">
  <img alt="stream" src="https://img.shields.io/badge/stream-H.264%20%7C%2060fps-1464a0">
  <a href="LICENSE"><img alt="license" src="https://img.shields.io/badge/license-GPL--3.0-blue"></a>
</p>

## ما هو PBOX؟

PBOX هو عميل ألعاب سحابية مخصص لجهاز PlayStation 4 معدل. يعرض مكتبة الألعاب، يطلب جلسة البث، ويرسل تحكم DualShock 4 أو لوحة المفاتيح والماوس إلى الجلسة السحابية. المشروع **ليس محاكيًا** ولا ينسخ ألعاب Xbox أو PlayStation، ولا يوزّع محتوى الألعاب.

يتطلب استخدام PBOX حساب Microsoft وخطة Game Pass تتضمن اللعب السحابي، كما يتطلب جهاز PS4 متوافقًا مع بيئة homebrew وGoldHEN. توفر الخدمة وجودة الاتصال يعتمدان على البلد وشبكة الإنترنت ومسار الخادم.

## أبرز المميزات

- واجهة PBOX جديدة داكنة مع هوية بصرية مستقلة وبطاقات إعدادات واضحة.
- بث H.264 مع عرض غير متزامن على PS4 لتقليل انتظار الصورة.
- ضبط تلقائي للتعامل مع RTT وفقدان الحزم واهتزاز الشبكة.
- NACK وJitter Buffer وRTCP لاستعادة الحزم قدر الإمكان عند تذبذب الشبكة.
- إعادة اتصال تلقائية بعد الانقطاعات القصيرة.
- إعدادات للدقة والـ bitrate وفك ترميز الفيديو والصوت.
- دعم USB DualShock 4؛ إدخال الماوس والكيبورد الكامل على PS4 يحتاج طبقة HID أصلية إضافية، لذلك لا نعد بدعم كامل لهما بعد.
- شاشة إحصائيات تعرض FPS والـ bitrate والكمون وفقدان الحزم والإطارات المسقطة.
- WebUI للتحكم عن بعد وتعديل إعدادات البث وإعادة تعيين أزرار التحكم.
- حركة افتتاحية أصلية خفيفة لـPBOX تعتمد على نبض الشعار والخطوط المضيئة ولا تحتاج ملف فيديو خارجي.
- حفظ الإعدادات وبيانات تسجيل الدخول خارج ملفات التطبيق حتى لا تضيع عادةً عند التحديث.
- خيار Best يختار أعلى tier متاح، مع رفع الصورة إلى شاشة 4K عبر GPU عند دعمها؛ هذا لا يعني أن خادم xCloud سيرسل بثًا أصليًا بدقة 4K.

## الإصدار الجاري: PBOX v1.6.4

يتضمن `v1.6.4` تقليل طابور الفيديو إلى 48 إطارًا لتحسين زمن الاستجابة عند تذبذب الشبكة، وتحسين واجهة WebUI العمودية. يحتفظ بخفض تلقائي لدقة البث إلى 720 عند استمرار فقدان الإطارات أو ارتفاع RTT أو توقف الفيديو، ويحمي خيوط العرض والفيديو والصوت والإدخال من الاستثناءات.

لا أستطيع ضمان عدم حدوث أي خروج بنسبة 100% قبل اختبار PKG على جهاز PS4 فعلي، لكن مسارات الانهيار المعروفة عند بدء اللعبة وأثناء أول الحزم محمية الآن.

يتضمن الإصدار `v1.6.8` إصلاحًا مهمًا لمسار دخول اللعبة: استثناءات WebRTC أو أول دفعة فيديو/صوت لا تنهي التطبيق الآن، بل تتحول إلى خطأ قابل للتشخيص أو استعادة عبر keyframe وإعادة الاتصال.

كما أن تحسينات الأداء الموجودة فعليًا تشمل REMB وNACK وRTCP وطابور الإطارات والعرض غير المتزامن وإعادة الاتصال ومراقبة FPS/RTT/Bitrate/Loss. لم نضف Smart FPS أو Quick Boost كأزرار مستقلة لأن المشروع لا يملك API آمنًا لتغيير معدل إطارات الخادم أو تطبيق إعداد شامل لحظيًا.

يتضمن الإصدار الأخير:

- إعادة تصميم شاشة الإعدادات بأسلوب Control Centre مع أقسام وفواصل وألوان PBOX.
- إبراز إعدادات الأداء والإدخال للوصول إليها باليد أو بالماوس والكيبورد.
- تقليل تراكم الإطارات القديمة من 72 إلى 48 إطارًا لتقليل التأخير بعد تذبذب 4G أو 5G.
- خفض حد إعادة مزامنة الفيديو على PS4 من 250ms إلى 220ms.
- الحفاظ على العرض غير المتزامن وNACK وRTCP وإعادة الاتصال والتشخيص.

## التحميل والتثبيت على PS4

1. افتح صفحة Release الخاصة بـPBOX بعد نجاح بناء `v1.6.1`.
2. حمّل ملف `PBOX-PS4-v1.6.1.pkg` وملف `PBOX-PS4-v1.6.1.pkg.sha256`.
3. انسخ ملف PKG إلى جذر USB مهيأ بنظام exFAT أو FAT32.
4. أدخل USB في PS4.
5. من GoldHEN افتح Package Installer وثبّت الحزمة فوق النسخة السابقة.
6. لا تحذف بيانات التطبيق قبل التحديث؛ تثبيت الحزمة فوق النسخة السابقة يحافظ على البيانات والإعدادات عادةً.

[صفحة جميع إصدارات PBOX](https://github.com/noan511ee-dot/Xbox/releases)

> تنبيه: `v1.6.1` أقل من `v1.6.9` السابق، لذلك قد يرفض PS4 تثبيته فوق النسخة الأحدث باعتباره Downgrade. عند ظهور هذا الرفض سنستخدم رقمًا أعلى مثل `v1.6.10` بدلًا منه.

## شبكة 4G و5G

لا يحتاج PBOX إلى وضع خاص باسم 4G أو 5G. اتصل بالـPS4 من نقطة اتصال الهاتف أو من الراوتر، وسيحاول التطبيق التكيف مع سرعة الاتصال. للحصول على أفضل نتيجة:

- استخدم 5G عندما تكون الإشارة مستقرة، وإلا فـ4G ثابتة أفضل من 5G متذبذبة.
- قرّب الهاتف من مكان اللعب وأوقف التنزيلات والنسخ الاحتياطي أثناء اللعب.
- ابدأ بدقة 720p أو bitrate تلقائي إذا كان الاتصال غير مستقر.
- راقب FPS والـ RTT وفقدان الحزم من شاشة الإحصائيات.

## التوافق

هذه حزمة اختبارية لـPS4. لا يوجد ضمان أن نفس PKG سيعمل على كل Firmware من 9.00 إلى 14.00؛ التوافق يعتمد على GoldHEN وإصدار النظام وواجهات OpenOrbis المستخدمة. اختبر الحزمة على جهازك قبل الاعتماد عليها، ولا تحدّث Firmware الجهاز فقط من أجل هذا المشروع.

يوجد ملف حالة منفصل لمسار PS5 في [`ps5/PBOX-PS5-STATUS.md`](ps5/PBOX-PS5-STATUS.md). لا يمثل هذا الملف PKG PS5 جاهزًا؛ بناء PS5 يحتاج بيئة وأدوات PS5 مستقلة.

## سياسة الإصدارات

كل إصدار جديد يجب أن يحتوي على رقم نسخة واضح، وقائمة بالمميزات أو الإصلاحات الفعلية، وملف PKG، وSHA256، ونتيجة GitHub Actions. الإصدارات المستقبلية يجب أن تضيف مميزاتها في [CHANGELOG.md](CHANGELOG.md) وفي ملاحظات Release، لا أن تغيّر الرقم فقط.

## البناء عبر GitHub Actions

يوجد Workflow جاهز في `.github/workflows/build-ps4-pkg.yml` ويستخدم OpenOrbis Toolchain:

1. افتح تبويب **Actions** في GitHub.
2. اختر **Build PBOX PS4 PKG**.
3. اضغط **Run workflow**.
4. أدخل رقم النسخة مثل `1.6.8`.
5. بعد النجاح حمّل Artifact، أو أنشئ Tag مثل `v1.6.8` لإصدار Release.

للبناء المحلي، يلزم CMake 3.20 أو أحدث وNinja وClang/LLVM وPython وOpenOrbis Toolchain:

```bash
git clone --recursive https://github.com/noan511ee-dot/Xbox.git
cd Xbox
tools/ps4/fetch-toolchain.sh
cmake -S . -B build-ps4 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ps4.toolchain.cmake
ninja -C build-ps4
tools/ps4/package.sh build-ps4
```

## الإبلاغ عن مشكلة

قبل فتح Issue، جرّب تشغيل اختبار الشبكة من إعدادات PBOX، ثم أرسل Firmware الجهاز وإصدار GoldHEN ورقم PBOX ولقطة من الإحصائيات أو السجل. لا ترسل `account.json` أو رموز تسجيل الدخول أو أي بيانات شخصية.

افتح [Issue جديدًا](https://github.com/noan511ee-dot/Xbox/issues/new) أو تابع [GitHub Actions](https://github.com/noan511ee-dot/Xbox/actions).

## الترخيص والحقوق

المشروع مرخص تحت GPL-3.0-or-later. PBOX مشروع غير رسمي وغير تابع لـMicrosoft أو Sony. Xbox وXbox Cloud Gaming وGame Pass علامات تجارية لمجموعة Microsoft، وPlayStation وPS4 علامات تجارية لشركة Sony Interactive Entertainment.
