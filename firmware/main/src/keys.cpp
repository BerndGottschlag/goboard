#include "keys.hpp"

template<class KeyMatrixType>
Keys<KeyMatrixType>::Keys(KeyMatrixType &key_matrix): key_matrix(key_matrix) {
	// TODO
}

template<class KeyMatrixType>
Keys<KeyMatrixType>::~Keys() {
	// TODO
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::get_state(KeyBitmap &state) {
	// TODO
}

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
#include "key_matrix.hpp"
template class Keys<KeyMatrix>;
#endif

