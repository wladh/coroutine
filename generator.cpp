#include <iostream>
#include <experimental/coroutine>

// Simple generator coroutine type base on "co_yield"
struct generator {
	struct promise_type;
	using handle = std::experimental::coroutine_handle<promise_type>;

	// Promise type needs to implement a specific interface
	struct promise_type {
		int current_value;

		// Coroutine return oject
		auto get_return_object() { return generator{handle::from_promise(*this)}; }
		// This will be called automatically before the coroutine body is executed
		// (eg the equivalent of "co_await initial_suspend()")
		auto initial_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically after the coroutine has terminated
		auto final_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically if unhandled exception are thrown by the coroutine.
		void unhandled_exception() { std::terminate(); }

		// This is the equivalent of "co_return" (with no value) at the end of each coroutine body.
		// We need either to implement this, or have a "co_return" (with or without value)
		// in each coroutine.
		// Flowing off a coroutine without a "co_return" or "return_void" implement is UB.
		void return_void() {}

		// Called every time the coroutine yields (via "co_yield")
		auto yield_value(int value) {
			current_value = value;
			// We suspend again after storing the yielded value
			return std::experimental::suspend_always{};
		}
	};

	// A simple iterator so we can use the generator in a for loop.
	// We could have gone range-based here, but we'll keep it simple.
	class iterator {
		generator &owner;
		bool done;
		void advance() {
			// Resume the coroutine so we get the next value
			owner.coro.resume();
			done = owner.coro.done();
		}
		public:
		iterator(generator &o, bool d) : owner(o), done(d) { if (!done) advance(); }
		bool operator !=(const iterator &r) const {
			return done != r.done;
		}
		iterator &operator++() {
			advance();
			return *this;
		}
		int operator*() const { return owner.coro.promise().current_value; }
	};
	iterator begin() { return iterator{*this, false}; }
	iterator end() { return iterator{*this, true}; }

	// We only allow moves, so that we can safely destroy the coroutine when
	// the generator gets destroyed.
	generator(generator const&) = delete;
	generator(generator && rhs) : coro(rhs.coro) { rhs.coro = nullptr; }
	~generator() { if (coro) coro.destroy(); }

	private:
	generator(handle h) : coro(h) {}
	handle coro;
};

// Simple infinite generator that yields consecutive integers.
generator f() {
	for (auto i = 1;; i++) co_yield i;
}

int main() {
	for (auto v: f()) {
		std::cout << v << std::endl;
		if (v >= 20) break;
	}
}
