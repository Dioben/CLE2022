/**
 * @file utfUtils.c (implementation file)
 *
 * @brief Problem name: multiprocess word count
 *
 * Defines some components needed by both worker and dispatcher.
 *
 * @author Pedro Casimiro, nmec: 93179
 * @author Diogo Bento, nmec: 93391
 */

#include <stdbool.h>
#include "utfUtils.h"

/**
 * @brief Returns if UTF-8 character is a bridge character.
 *
 * @param letter UTF-8 character
 * @return if character is a bridge
 */
bool isBridge(int letter)
{
    return (
        letter == 0x27                              // '
        || letter == 0x60                           // `
        || 0xE28098 == letter || letter == 0xE28099 //  ’
    );
}
/**
 * @brief Returns if UTF-8 character is a vowel.
 *
 * @param letter UTF-8 character
 * @return if character is a vowel
 */
bool isVowel(int letter)
{
    return (
        letter == 0x41                            // A
        || letter == 0x45                         // E
        || letter == 0x49                         // I
        || letter == 0x4f                         // O
        || letter == 0x55                         // U
        || letter == 0x61                         // a
        || letter == 0x65                         // e
        || letter == 0x69                         // i
        || letter == 0x6f                         // o
        || letter == 0x75                         // u
        || (0xc380 <= letter && letter <= 0xc383) // À Á Â Ã
        || (0xc388 <= letter && letter <= 0xc38a) // È É Ê
        || (0xc38c == letter || letter == 0xc38d) // Ì Í
        || (0xc392 <= letter && letter <= 0xc395) // Ò Ó Ô Õ
        || (0xc399 == letter || letter == 0xc39a) // Ù Ú
        || (0xc3a0 <= letter && letter <= 0xc3a3) // à á â ã
        || (0xc3a8 <= letter && letter <= 0xc3aa) // è é ê
        || (0xc3ac == letter || letter == 0xc3ad) // ì í
        || (0xc3b2 <= letter && letter <= 0xc3b5) // ò ó ô õ
        || (0xc3b9 == letter || letter == 0xc3ba) // ù ú
    );
}
/**
 * @brief Returns if UTF-8 character is a consonant.
 *
 * @param letter UTF-8 character
 * @return if character is a consonant
 */
bool isConsonant(int letter)
{
    return (
        letter == 0xc387                      // Ç
        || letter == 0xc3a7                   // ç
        || (0x42 <= letter && letter <= 0x44) // B C D
        || (0x46 <= letter && letter <= 0x48) // F G H
        || (0x4a <= letter && letter <= 0x4e) // J K L M N
        || (0x50 <= letter && letter <= 0x54) // P Q R S T
        || (0x56 <= letter && letter <= 0x5a) // V W X Y Z
        || (0x62 <= letter && letter <= 0x64) // b c d
        || (0x66 <= letter && letter <= 0x68) // f g h
        || (0x6a <= letter && letter <= 0x6e) // j k l m n
        || (0x70 <= letter && letter <= 0x74) // p q r s t
        || (0x76 <= letter && letter <= 0x7a) // v w x y z
    );
}
/**
 * @brief Returns if UTF-8 character is a separator character.
 *
 * @param letter UTF-8 character
 * @return if character is a separator
 */
bool isSeparator(int letter)
{
    return (
        letter == 0x20                              // space
        || letter == 0x9                            // \t
        || letter == 0xA                            // \n
        || letter == 0xD                            // \r
        || letter == 0x5b                           // [
        || letter == 0x5d                           // ]
        || letter == 0x3f                           // ?
        || letter == 0xc2ab                         // «
        || letter == 0xc2bb                         // »
        || letter == 0xe280a6                       // …
        || 0x21 == letter || letter == 0x22         // ! "
        || 0x28 == letter || letter == 0x29         // ( )
        || (0x2c <= letter && letter <= 0x2e)       // , - .
        || 0x3a == letter || letter == 0x3b         // : ;
        || 0xe28093 == letter || letter == 0xe28094 // – —
        || 0xe2809c == letter || letter == 0xe2809d // “ ”
    );
}