/**
 * @file utfUtils.h (interface file)
 *
 * @brief Problem name: multiprocess word count
 *
 * Defines some components needed by both worker and dispatcher.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */


/** @brief If byte is a start of an UTF-8 character and implies a 1 byte length character. */
#define byte0utf8(byte) !(byte >> 7)

/** @brief If byte is a start of an UTF-8 character and implies a 2 byte length character. */
#define byte1utf8(byte) (byte >= 192 && byte <= 223)

/** @brief If byte is a start of an UTF-8 character and implies a 3 byte length character. */
#define byte2utf8(byte) (byte >= 224 && byte <= 239)

/** @brief If byte is a start of an UTF-8 character and implies a 4 byte length character. */
#define byte3utf8(byte) (byte >= 240 && byte <= 247)


/**
 * @brief Returns if UTF-8 character is a bridge character.
 *
 * @param letter UTF-8 character
 * @return if character is a bridge
 */
bool isBridge(int letter);

/**
 * @brief Returns if UTF-8 character is a vowel.
 *
 * @param letter UTF-8 character
 * @return if character is a vowel
 */
bool isVowel(int letter);

/**
 * @brief Returns if UTF-8 character is a consonant.
 *
 * @param letter UTF-8 character
 * @return if character is a consonant
 */
bool isConsonant(int letter);

/**
 * @brief Returns if UTF-8 character is a separator character.
 *
 * @param letter UTF-8 character
 * @return if character is a separator
 */
bool isSeparator(int letter);