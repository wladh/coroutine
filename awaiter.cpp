#include <iostream>
#include <experimental/coroutine>

// This coroutine type synchronously returns a value
// (ie, it resumes the coroutine and returns the produced value)
struct sync {
	struct promise_type;
	using handle = std::experimental::coroutine_handle<promise_type>;

	struct promise_type {
		int value;

		// Coroutine return oject
		auto get_return_object() { return sync{handle::from_promise(*this)}; }
		// This will be called automatically before the coroutine body is executed
		// (eg the equivalent of "co_await initial_suspend()")
		auto initial_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically after the coroutine has terminated
		auto final_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically if unhandled exception are thrown by the coroutine.
		void unhandled_exception() { std::terminate(); }

		// Called when the coroutine executes "co_return <value>"
		auto return_value(int v) {
			value = v;
			// We won't suspend again here because we're done
			return std::experimental::suspend_never{};
		}
	};

	// Resume the coroutine and return the produced value
	// (initial_suspend() returned suspend_always, so the coroutine starts suspended)
	int get() {
		this->coro.resume();
		return coro.promise().value;
	}

	// We only allow moves, so that we can safely destroy the coroutine when
	// the generator gets destroyed.
	sync(sync const&) = delete;
	sync(sync && rhs) : coro(rhs.coro) { rhs.coro = nullptr; }
	~sync() { if (coro) coro.destroy(); }

	private:
	sync(handle h) : coro(h) {}
	handle coro;
};

// Simple coroutine type that implements an async task (ie, one that can be co_await'ed)
struct task {
	struct promise_type;
	using handle = std::experimental::coroutine_handle<promise_type>;

	struct promise_type {
		int value;

		// Coroutine return oject
		auto get_return_object() { return task{handle::from_promise(*this)}; }
		// This will be called automatically before the coroutine body is executed
		// (eg the equivalent of "co_await initial_suspend()")
		auto initial_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically after the coroutine has terminated
		auto final_suspend() { return std::experimental::suspend_always{}; }
		// This will be called automatically if unhandled exception are thrown by the coroutine.
		void unhandled_exception() { std::terminate(); }

		// Called when the coroutine executes "co_return <value>"
		auto return_value(int v) {
			value = v;
			// We won't suspend again here because we're done
			return std::experimental::suspend_never{};
		}
	};

	// The three methods belows need to be implement by an a coroutine type that can be awaited.
	// This method returns true when the promised value is ready.
	bool await_ready() {
		// We're ready once the coroutine is done (ie, it called "co_return <value>")
		return coro.done();
	}
	// This method is called when co_await suspends.
	// Its argument is the coroutine that called co_await.
	void await_suspend(std::experimental::coroutine_handle<> awaiting) {
		// Resume the producer coroutine so that it produces the value
		coro.resume();
		// Resume the awaiting coroutine as it can use the value now
		awaiting.resume();
	}
	// This method is called after await_ready returns true to retrieve the value.
	auto await_resume() {
		const auto r = coro.promise().value;
		return r;
	}

	// We only allow moves, so that we can safely destroy the coroutine when
	// the generator gets destroyed.
	task(task const&) = delete;
	task(task && rhs) : coro(rhs.coro) { rhs.coro = nullptr; }
	~task() { if (coro) coro.destroy(); }

	private:
	task(handle h) : coro(h) {}
	handle coro;
};

// A simple task that returns a value.
task t() {
	co_return 1;
}

// A sync task that awaits a value from another coroutine and prints it out.
// We use co_return at the end because this is a coroutine itself
// (any function using co_await, co_return, co_yield is a coroutine).
// main() can't be a coroutine so we can't fold body of this function into main.
sync s() {
	auto c = t();
	auto v = co_await c;

	std::cout << v << std::endl;

	co_return 0;
}

int main() {
	return s().get();
}
