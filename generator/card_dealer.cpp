//
// card_dealer.cpp
// ---------------
//
// g++-11 -fcoroutines -Wall -Werror card_dealer.cpp -o dealer
//

#include <coroutine>
#include <random>
#include <array>
#include <string>

#include <iostream>

template <typename T> struct generator {
  struct promise_type;
  using coroutine_handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    T current_value;

    auto get_return_object() {
      return generator{coroutine_handle::from_promise(*this)};
    }

    // Start 'lazily' i.e. suspend ('eagerly' == 'suspend_never').
    auto initial_suspend() { return std::suspend_always{}; }

    // Opportunity to publish results, signal completion etc. Suspend
    // so that destroy() is called via RAII from outside the coroutine.
    auto final_suspend() noexcept { return std::suspend_always{}; }

    void unhandled_exception() { std::terminate(); }

    auto yield_value(T const &value) {
      current_value = value;
      return std::suspend_always{};
    }
  };

  bool next() {
    return coroutine ? (coroutine.resume(), !coroutine.done())
                     : false;
  }

  T value() { return coroutine.promise().current_value; }

  // Range support, class design etc.

  ~generator() {
    if (coroutine)
      coroutine.destroy();
  }

private:
  generator(coroutine_handle h) : coroutine(h) {}
  coroutine_handle coroutine;
};

generator<std::string> card_dealer(int deck_size) {
  std::default_random_engine rng;
  std::uniform_int_distribution<int> card(0, 12);
  std::uniform_int_distribution<int> suit(0, 3);

  std::array<std::string, 13> cards = {
      "Ace", "2", "3", "4", "5", "6", "7",
      "8", "9", "10", "Jack", "Queen", "King"};

  std::array<std::string, 4> suits = {"Clubs", "Diamonds",
                                      "Spades", "Hearts"};

  for (int i = 0; i < deck_size; i++)
    co_yield(cards[card(rng)] + " of " + suits[suit(rng)]);
}

int main() {
  generator<std::string> dealer = card_dealer(100);

  while (dealer.next())
    std::cout << dealer.value() << std::endl;
}
