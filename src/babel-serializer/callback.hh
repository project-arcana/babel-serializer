#pragma once

#include <clean-core/function_ref.hh>

namespace babel
{
enum class callback_behavior
{
    continue_, ///< continues processing (default behavior)
    break_     ///< stops further processing
};

namespace detail
{
struct void_callback_invoker
{
    template <class F, class... Args>
    static constexpr callback_behavior invoke(F&& f, Args&&... args)
    {
        f(cc::forward<Args>(args)...);
        return callback_behavior::continue_;
    }
};
}

/// a cc::function_ref with callback specific features.
///
/// accepts
///   cc::function_ref<void(Args...)>
///   cc::function_ref<callback_behavior(Args...)>
///
/// the void version is equivalent to "return callback_behavior::continue_"
///
/// default constructed callbacks are well defined, do nothing, and always continue
template <class... Args>
struct callback
{
    constexpr callback_behavior operator()(Args... args) const { return _fun(cc::forward<Args>(args)...); }

    /// no-op callback, always continues
    constexpr callback()
    {
        _fun = [](Args...) { return callback_behavior::continue_; };
    }

    template <class F, cc::enable_if<std::is_invocable_v<F, Args...> && !std::is_same_v<std::decay_t<F>, callback>> = true>
    constexpr callback(F&& f)
    {
        using R = std::decay_t<std::invoke_result_t<F, Args...>>;

        if constexpr (std::is_same_v<R, void>)
            _fun = _fun.template with_custom_invoker<detail::void_callback_invoker>(f);
        else if constexpr (std::is_same_v<R, callback_behavior>)
            _fun = f;
        else
            static_assert(cc::always_false<F, R>, "callback must return void or callback_behavior. "
                                                  "note that bool is not allowed due to ambiguous interpretation.");
    }

private:
    cc::function_ref<callback_behavior(Args...)> _fun;
};
}
