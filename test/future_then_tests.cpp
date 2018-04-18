/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#include <boost/test/unit_test.hpp>

#include <stlab/concurrency/default_executor.hpp>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/utility.hpp>
#include <stlab/test/model.hpp>

#include "future_test_helper.hpp"

using namespace stlab;
using namespace future_test_helper;

BOOST_FIXTURE_TEST_SUITE(future_then_void, test_fixture<void>)

BOOST_AUTO_TEST_CASE(future_void_single_task) {
    BOOST_TEST_MESSAGE("running future void single task");

    int p = 0;

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_single_task_detached) {
    BOOST_TEST_MESSAGE("running future void single task detached");

    std::atomic_int p{0};
    {
        auto detached = async(custom_scheduler<0>(), [& _p = p] { _p = 42; });
        detached.detach();
    }
    while (p.load() != 42) {
    }
}

BOOST_AUTO_TEST_CASE(future_void_two_tasks_with_same_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE("running future void with two task on same scheduler, then on r-value");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; }).then([& _p = p] { _p += 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_two_tasks_with_same_scheduler_then_on_lvalue) {
    BOOST_TEST_MESSAGE("running future void with two task on same scheduler, then on l-value");

    std::atomic_int p{0};
    auto interim = async(custom_scheduler<0>(), [& _p = p] { _p = 42; });

    sut = interim.then([& _p = p] { _p += 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_void_two_tasks_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future int void tasks with same scheduler");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [] { return 42; }).then([& _p = p](auto x) { _p = x + 42; });
    check_valid_future(sut);

    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_void_two_tasks_with_different_scheduler) {
    BOOST_TEST_MESSAGE("running future int void tasks with different schedulers");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [] { return 42; })
              .then(custom_scheduler<1>(), [& _p = p](auto x) { _p = x + 42; });
    check_valid_future(sut);

    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_two_tasks_with_different_scheduler) {
    BOOST_TEST_MESSAGE("running future void two tasks with different schedulers");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; })
              .then(custom_scheduler<1>(), [& _p = p] { _p += 42; });
    check_valid_future(sut);

    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

/*
        f1
       /
    sut
       \
        f2
*/
BOOST_AUTO_TEST_CASE(future_void_Y_formation_tasks_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future void with Y formation with same scheduler");

    std::atomic_int p{0};
    int r1 = 0;
    int r2 = 0;

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; });
    auto f1 = sut.then(custom_scheduler<0>(), [& _p = p, &_r = r1] { _r = 42 + _p; });
    auto f2 = sut.then(custom_scheduler<0>(), [& _p = p, &_r = r2] { _r = 4711 + _p; });

    check_valid_future(sut, f1, f2);
    wait_until_future_completed(f1, f2);

    BOOST_REQUIRE_EQUAL(42 + 42, r1);
    BOOST_REQUIRE_EQUAL(42 + 4711, r2);
    BOOST_REQUIRE_LE(3, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(reduction_future_void) {
    BOOST_TEST_MESSAGE("running future reduction void to void");

    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor, [& _flag = first] { _flag = true; }).then([& _flag = second] {
        return async(default_executor, [&_flag] { _flag = true; });
    });

    wait_until_future_completed(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
}

BOOST_AUTO_TEST_CASE(reduction_future_int_to_void) {
    BOOST_TEST_MESSAGE("running future reduction int to void");
    std::atomic_bool first{false};
    std::atomic_bool second{false};
    std::atomic_int result{0};

    sut = async(default_executor,
                [& _flag = first] {
                    _flag = true;
                    return 42;
                })
              .then([& _flag = second, &_result = result](auto x) {
                  return async(default_executor,
                               [&_flag, &_result](auto x) {
                                   _flag = true;
                                   _result = x + 42;
                               },
                               x);
              });

    wait_until_future_completed(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
    BOOST_REQUIRE_EQUAL(84, result);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_then_non_copyable, test_fixture<stlab::move_only>)
BOOST_AUTO_TEST_CASE(future_non_copyable_single_task) {
    BOOST_TEST_MESSAGE("running future non copyable single task");

    sut = async(custom_scheduler<0>(), [] { return move_only(42); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_then_non_copyable_detach) {
    BOOST_TEST_MESSAGE("running future non copyable, detached");
    std::atomic_bool check{false};
    {
        async(custom_scheduler<0>(),
              [& _check = check] {
                  _check = true;
                  return move_only(42);
              })
            .detach();
    }
    while (!check) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

BOOST_AUTO_TEST_CASE(future_non_copyable_capture) {
    BOOST_TEST_MESSAGE("running future non copyable capture");

    stlab::v1::move_only m{42};

    sut = async(custom_scheduler<0>(), [& _m = m] { return move_only(_m.member()); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(
    future_copyable_with_non_copyable_as_continuation_with_same_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE(
        "running future copyable with non copyable as contination with same scheduler, then on r-value");

    sut = async(custom_scheduler<0>(), [] { return 42; }).then([](auto x) { return move_only(x); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(
    future_copyable_with_non_copyable_as_continuation_with_different_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE(
        "running future copyable with non copyable as contination with different scheduler, then on r-value");

    sut = async(custom_scheduler<0>(), [] { return 42; }).then(custom_scheduler<1>(), [](auto x) {
        return move_only(x);
    });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(
    future_copyable_with_non_copyable_as_continuation_with_same_scheduler_then_on_lvalue) {
    BOOST_TEST_MESSAGE(
        "running future copyable with non copyable as contination with same scheduler, then on l-value");

    auto interim = async(custom_scheduler<0>(), [] { return 42; });

    sut = interim.then([](auto x) { return move_only(x); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(
    future_copyable_with_non_copyable_as_continuation_with_different_scheduler_then_on_lvalue) {
    BOOST_TEST_MESSAGE(
        "running future copyable with non copyable as contination with different scheduler, then on l-value");

    auto interim = async(custom_scheduler<0>(), [] { return 42; });

    sut = interim.then(custom_scheduler<1>(), [](auto x) { return move_only(x); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_non_copyable_as_continuation_with_same_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE(
        "running future non copyable as contination with same scheduler, then on r-value");

    sut = async(custom_scheduler<0>(), [] { return move_only(42); }).then([](auto x) {
        return move_only(x.member() * 2);
    });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42 * 2, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_non_copyable_as_continuation_with_different_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE(
        "running future non copyable as contination with different scheduler, then on r-value");

    sut = async(custom_scheduler<0>(), [] { return move_only(42); })
              .then(custom_scheduler<1>(), [](auto x) { return move_only(x.member() * 2); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42 * 2, result->member());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_EQUAL(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_then_move_only, test_fixture<stlab::v1::move_only>)

BOOST_AUTO_TEST_CASE(future_async_move_only_move_captured_to_result) {
    BOOST_TEST_MESSAGE("running future move only move to result");

    sut = async(custom_scheduler<0>(), [] { return move_only{42}; }).then([](auto x) {
        return std::move(x);
    });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_async_moving_move_only_capture_to_result) {
    BOOST_TEST_MESSAGE("moving move_only capture to result");

    stlab::v1::move_only m{42};

    sut = async(custom_scheduler<0>(), [& _m = m] { return std::move(_m); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_async_mutable_move_move_only_capture_to_result) {
    BOOST_TEST_MESSAGE("moving move_only capture to result in mutable task");

    stlab::v1::move_only m{42};

    sut = async(custom_scheduler<0>(), [& _m = m]() mutable { return std::move(_m); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_continuation_moving_move_only_capture_to_result) {
    BOOST_TEST_MESSAGE("moving move_only capture to result");

    stlab::v1::move_only m{42};

    sut = async(custom_scheduler<0>(), [] { return stlab::v1::move_only{10}; })
              .then([& _m = m](auto x) mutable { return std::move(_m); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_continuation_async_mutable_move_move_only_capture_to_result) {
    BOOST_TEST_MESSAGE("moving move_only capture to result in mutable task");

    stlab::v1::move_only m{42};

    sut = async(custom_scheduler<0>(), []() mutable { return stlab::v1::move_only{10}; })
              .then([& _m = m](auto x) mutable { return std::move(_m); });

    check_valid_future(sut);
    auto result = wait_until_future_r_completed(sut);

    BOOST_REQUIRE_EQUAL(42, result->member());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_then_int, test_fixture<int>)

BOOST_AUTO_TEST_CASE(future_int_single_task) {
    BOOST_TEST_MESSAGE("running future int single tasks");

    sut = async(custom_scheduler<0>(), [] { return 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42, *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_single_task_get_try_on_rvalue) {
    BOOST_TEST_MESSAGE("running future int single tasks, get_try on r-value");

    sut = async(custom_scheduler<0>(), [] { return 42; });

    auto test_result_1 = std::move(sut).get_try(); // test for r-value implementation
    (void)test_result_1;
    wait_until_future_completed(sut);
    auto test_result_2 = std::move(sut).get_try();

    BOOST_REQUIRE_EQUAL(42, *sut.get_try());
    BOOST_REQUIRE_EQUAL(42, *test_result_2);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_single_task_detached) {
    BOOST_TEST_MESSAGE("running future int single tasks, detached");
    std::atomic_bool check{false};
    {
        auto detached = async(custom_scheduler<0>(), [& _check = check] {
            _check = true;
            return 42;
        });
        detached.detach();
    }
    while (!check) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

BOOST_AUTO_TEST_CASE(future_int_two_tasks_with_same_scheduler_then_on_rvalue) {
    BOOST_TEST_MESSAGE("running future int two tasks with same scheduler, then on r-value");

    sut = async(custom_scheduler<0>(), [] { return 42; }).then([](auto x) { return x + 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_two_tasks_with_same_scheduler_then_on_lvalue) {
    BOOST_TEST_MESSAGE("running future int two tasks with same scheduler, then on l-value");

    auto interim = async(custom_scheduler<0>(), [] { return 42; });

    sut = interim.then([](auto x) { return x + 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_two_tasks_with_different_scheduler) {
    BOOST_TEST_MESSAGE("running future int two tasks with different scheduler");

    sut = async(custom_scheduler<0>(), [] { return 42; }).then(custom_scheduler<1>(), [](auto x) {
        return x + 42;
    });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_int_two_tasks_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future void int tasks with same scheduler");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; }).then([& _p = p] {
        _p += 42;
        return _p.load();
    });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_int_two_tasks_with_different_scheduler) {
    BOOST_TEST_MESSAGE("running future void int tasks with different schedulers");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; })
              .then(custom_scheduler<1>(), [& _p = p] {
                  _p += 42;
                  return _p.load();
              });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

/*
    sut - f - f
*/
BOOST_AUTO_TEST_CASE(future_int_three_tasks_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future int with three tasks with same scheduler");

    sut = async(custom_scheduler<0>(), [] { return 42; })
              .then(custom_scheduler<0>(), [](auto x) { return x + 42; })
              .then(custom_scheduler<0>(), [](auto x) { return x + 42; });

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(3, custom_scheduler<0>::usage_counter());
}

/*
        f1
       /
    sut
       \
        f2
*/
BOOST_AUTO_TEST_CASE(future_int_Y_formation_tasks_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future int Y formation tasks with same scheduler");

    sut = async(custom_scheduler<0>(), [] { return 42; });
    auto f1 = sut.then(custom_scheduler<0>(), [](auto x) -> int { return x + 42; });
    auto f2 = sut.then(custom_scheduler<0>(), [](auto x) -> int { return x + 4177; });

    check_valid_future(sut, f1, f2);
    wait_until_future_completed(f1, f2);

    BOOST_REQUIRE_EQUAL(42 + 42, *f1.get_try());
    BOOST_REQUIRE_EQUAL(42 + 4177, *f2.get_try());
    BOOST_REQUIRE_LE(3, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(reduction_future_void_to_int) {
    BOOST_TEST_MESSAGE("running future reduction void to int");
    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor, [& _flag = first] { _flag = true; }).then([& _flag = second] {
        return async(default_executor, [&_flag] {
            _flag = true;
            return 42;
        });
    });

    wait_until_future_completed(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
    BOOST_REQUIRE_EQUAL(42, *sut.get_try());
}

BOOST_AUTO_TEST_CASE(reduction_future_int_to_int) {
    BOOST_TEST_MESSAGE("running future reduction int to int");
    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor,
                [& _flag = first] {
                    _flag = true;
                    return 42;
                })
              .then([& _flag = second](auto x) {
                  return async(default_executor,
                               [&_flag](auto x) {
                                   _flag = true;
                                   return x + 42;
                               },
                               x);
              });

    wait_until_future_completed(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
    BOOST_REQUIRE_EQUAL(84, *sut.get_try());
}
BOOST_AUTO_TEST_SUITE_END()

// ----------------------------------------------------------------------------
//                             Error cases
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(future_void_then_error, test_fixture<void>)
BOOST_AUTO_TEST_CASE(future_void_single_task_error) {
    BOOST_TEST_MESSAGE("running future void with single tasks that fails");

    sut = async(custom_scheduler<0>(), [] { throw test_exception("failure"); });

    wait_until_future_fails<test_exception>(sut);
    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_two_tasks_error_in_1st_task_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future void with two tasks which first fails");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [] { throw test_exception("failure"); }).then([& _p = p] {
        _p = 42;
    });

    wait_until_future_fails<test_exception>(sut);
    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_EQUAL(0, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_void_two_tasks_error_in_2nd_task_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future void with two tasks which second fails");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; }).then([& _p = p] {
        throw test_exception("failure");
    });

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_EQUAL(42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(reduction_future_void_to_void_error) {
    BOOST_TEST_MESSAGE("running future reduction void to void where the inner future fails");
    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor, [& _flag = first] { _flag = true; }).then([& _flag = second] {
        return async(default_executor, [&_flag] {
            _flag = true;
            throw test_exception("failure");
        });
    });

    wait_until_future_fails<test_exception>(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_then_int_error, test_fixture<int>)

BOOST_AUTO_TEST_CASE(future_int_single_task_error) {
    BOOST_TEST_MESSAGE("running future int with single tasks that fails");

    sut = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}
#if 0 // currently disabled, because I have doubts that get_try on an r-value makes any sense at al
    BOOST_AUTO_TEST_CASE(future_int_single_task_with_error_get_try_on_rvalue) {
        BOOST_TEST_MESSAGE("running future int single tasks, with error on get_try on r-value");

        sut = async(custom_scheduler<0>(), []()->int { throw test_exception("failure"); });
        auto test_result_1 = std::move(sut).get_try(); // test for r-value implementation
        wait_until_future_fails<test_exception>(sut);
        check_failure<test_exception>(std::move(sut), "failure");
        BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    }
#endif
BOOST_AUTO_TEST_CASE(future_int_two_tasks_error_in_1st_task_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future int with two tasks which first fails");
    int p = 0;

    sut = async(custom_scheduler<0>(), [] { throw test_exception("failure"); })
              .then([& _p = p]() -> int {
                  _p = 42;
                  return _p;
              });

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_EQUAL(0, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_two_tasks_error_in_2nd_task_with_same_scheduler) {
    BOOST_TEST_MESSAGE("running future void with two tasks which second fails");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), [& _p = p] { _p = 42; }).then([]() -> int {
        throw test_exception("failure");
    });

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_EQUAL(42, p);
    BOOST_REQUIRE_LE(2, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_Y_formation_tasks_with_failing_1st_task) {
    BOOST_TEST_MESSAGE("running future int Y formation tasks where the 1st tasks fails");

    std::atomic_int p{0};

    sut = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f1 = sut.then(custom_scheduler<0>(), [& _p = p](auto x) -> int {
        _p += 1;
        return x + 42;
    });
    auto f2 = sut.then(custom_scheduler<0>(), [& _p = p](auto x) -> int {
        _p += 1;
        return x + 4177;
    });

    wait_until_future_fails<test_exception>(f1, f2);

    check_failure<test_exception>(f1, "failure");
    check_failure<test_exception>(f2, "failure");
    BOOST_REQUIRE_EQUAL(0, p);
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_Y_formation_tasks_where_one_of_the_2nd_task_failing) {
    BOOST_TEST_MESSAGE("running future int Y formation tasks where one of the 2nd tasks fails");

    sut = async(custom_scheduler<0>(), []() -> int { return 42; });
    auto f1 = sut.then(custom_scheduler<0>(), [](auto) -> int { throw test_exception("failure"); });
    auto f2 = sut.then(custom_scheduler<0>(), [](auto x) -> int { return x + 4711; });

    wait_until_future_completed(f2);
    wait_until_future_fails<test_exception>(f1);

    check_failure<test_exception>(f1, "failure");
    BOOST_REQUIRE_EQUAL(42 + 4711, *f2.get_try());
    BOOST_REQUIRE_LE(3, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_int_Y_formation_tasks_where_both_of_the_2nd_task_failing) {
    BOOST_TEST_MESSAGE("running future int Y formation tasks where both of the 2nd tasks fails");

    sut = async(custom_scheduler<0>(), []() -> int { return 42; });
    auto f1 = sut.then(custom_scheduler<0>(), [](auto) -> int { throw test_exception("failure"); });
    auto f2 = sut.then(custom_scheduler<0>(), [](auto) -> int { throw test_exception("failure"); });

    wait_until_future_fails<test_exception>(f1, f2);

    check_failure<test_exception>(f1, "failure");
    check_failure<test_exception>(f2, "failure");
    BOOST_REQUIRE_LE(3, custom_scheduler<0>::usage_counter());
}

BOOST_AUTO_TEST_CASE(reduction_future_void_to_int_error) {
    BOOST_TEST_MESSAGE("running future reduction void to int where the outer future fails");
    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor, [& _flag = first] { _flag = true; })
              .then([& _flag = second]() -> future<int> { throw test_exception("failure"); });

    wait_until_future_fails<test_exception>(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(!second);
}

BOOST_AUTO_TEST_CASE(reduction_future_int_to_int_error) {
    BOOST_TEST_MESSAGE("running future reduction int to int where the inner future fails");
    std::atomic_bool first{false};
    std::atomic_bool second{false};

    sut = async(default_executor,
                [& _flag = first] {
                    _flag = true;
                    return 42;
                })
              .then([& _flag = second](auto x) {
                  return async(default_executor,
                               [&_flag](auto) -> int {
                                   _flag = true;
                                   throw test_exception("failure");
                               },
                               x);
              });

    wait_until_future_fails<test_exception>(sut);

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
}

BOOST_AUTO_TEST_SUITE_END()

#if 0
BOOST_AUTO_TEST_CASE(reduction_future_move_only_to_move_only) {
    BOOST_TEST_MESSAGE("running future reduction move-only to move-only");
    std::atomic_bool first{ false };
    std::atomic_bool second{ false };

    future<move_only> a = async(default_executor, [&_flag = first] {
        _flag = true; std::cout << 1 << std::endl; return move_only(42); }).then([&_flag = second] (auto&& x) {
        return async(default_executor, [&_flag] (auto&& x) {
            _flag = true; std::cout << 2 << std::endl; return std::forward<move_only>(x); }, std::forward<move_only>(x));
    });

    while (!a.get_try()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    BOOST_REQUIRE(first);
    BOOST_REQUIRE(second);
    BOOST_REQUIRE_EQUAL(42, a.get_try().value().member());
}
#endif
