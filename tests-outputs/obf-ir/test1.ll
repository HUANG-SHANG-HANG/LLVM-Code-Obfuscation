; ModuleID = '/home/hang/graduationDesign/tests-outputs/non-obf-ir/test1.ll'
source_filename = "../tests/test1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [21 x i8] c"compute(10, 3) = %d\0A\00", align 1
@.str.1 = private unnamed_addr constant [21 x i8] c"compute(3, 10) = %d\0A\00", align 1
@.str.2 = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @compute(i32 noundef %0, i32 noundef %1) #0 {
  call void @__obf_runtime_init()
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  store i32 0, ptr %5, align 4
  %6 = load i32, ptr %3, align 4
  %7 = load i32, ptr %4, align 4
  %8 = icmp sgt i32 %6, %7
  br i1 %8, label %sched., label %sched.1

sched.:                                           ; preds = %2
  %chaos.ok = call i1 @__obf_chaos_predicate()
  %thread.ok = call i1 @__obf_thread_predicate()
  %mixed = or i1 %chaos.ok, %thread.ok
  br i1 %mixed, label %9, label %fake.

fake.:                                            ; preds = %sched.
  br label %9

9:                                                ; preds = %fake., %sched.
  %10 = load i32, ptr %3, align 4
  %11 = load i32, ptr %4, align 4
  %12 = sub nsw i32 %10, %11
  store i32 %12, ptr %5, align 4
  br label %sched.6

sched.1:                                          ; preds = %2
  %chaos.ok3 = call i1 @__obf_chaos_predicate()
  %thread.ok4 = call i1 @__obf_thread_predicate()
  %mixed5 = or i1 %chaos.ok3, %thread.ok4
  br i1 %mixed5, label %13, label %fake.2

fake.2:                                           ; preds = %sched.1
  br label %13

13:                                               ; preds = %fake.2, %sched.1
  %14 = load i32, ptr %4, align 4
  %15 = load i32, ptr %3, align 4
  %16 = sub nsw i32 %14, %15
  store i32 %16, ptr %5, align 4
  br label %sched.6

sched.6:                                          ; preds = %9, %13
  %chaos.ok8 = call i1 @__obf_chaos_predicate()
  %thread.ok9 = call i1 @__obf_thread_predicate()
  %mixed10 = or i1 %chaos.ok8, %thread.ok9
  br i1 %mixed10, label %17, label %fake.7

fake.7:                                           ; preds = %sched.6
  br label %17

17:                                               ; preds = %fake.7, %sched.6
  %18 = load i32, ptr %5, align 4
  ret i32 %18
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  call void @__obf_runtime_init()
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %4 = call i32 @compute(i32 noundef 10, i32 noundef 3)
  store i32 %4, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %5)
  %7 = call i32 @compute(i32 noundef 3, i32 noundef 10)
  store i32 %7, ptr %3, align 4
  %8 = load i32, ptr %3, align 4
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.1, i32 noundef %8)
  %10 = call i32 (ptr, ...) @printf(ptr noundef @.str.2)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

declare i64 @__obf_array_idx(i64)

declare void @__obf_runtime_init()

declare i1 @__obf_chaos_predicate()

declare i1 @__obf_thread_predicate()

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
