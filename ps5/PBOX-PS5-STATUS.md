# PBOX for PlayStation 5

هذا الملف يعرّف مسار PS5 المنفصل داخل المشروع. لا يمثل PKG جاهزًا للتثبيت.

## الحالة الحالية

يوجد في المشروع كود PS5 وملفات `tools/ps5` و`ps5/release`، لكن بناء PS5 يحتاج بيئة PS5/Vulkan وأدوات التوقيع الخاصة به. لذلك لا يجوز تسمية ملف PS4 على أنه PKG PS5، ولا يجوز نشر حزمة PS5 قبل نجاح بناء واختبار مستقل.

## التصميم المخطط

- واجهة PBOX نفسها مع شعار PBOX الأصلي.
- إخراج 4K للشاشة عند توفره، مع بقاء دقة البث مرتبطة بما يمنحه xCloud فعليًا.
- FSR أو Anime4K على GPU، مع مراقبة FPS وRTT وBitrate.
- مسار إدخال DualSense منفصل عن مسار PS4.

## البناء

يظل البناء المحلي المرجعي:

```sh
ninja -C build-ps5 xcloud_app
bash tools/ps5/link.sh build-ps5
bash tools/ps5/package.sh build-ps5
```

لا يتم إنشاء Release PS5 من Workflow PS4. أي إصدار PS5 لاحق يجب أن يملك Workflow مستقلًا ونتيجة بناء واختبار واضحة.
