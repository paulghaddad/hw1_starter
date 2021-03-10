#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

const char* zero_commit_id =  "0000000000000000000000000000000000000000";

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  write_string_to_file(".beargit/.prev", zero_commit_id);

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 * 
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {
  // Open existing index
  FILE* fold_index;
  if ((fold_index = fopen(".beargit/.index", "r")) == NULL) {
    fprintf(stderr, "ERROR: Could not update index\n");
    return 1;
  }

  // Open new index
  FILE* fnew_index;
  if ((fnew_index = fopen(".beargit/.new_index", "w")) == NULL) {
    fprintf(stderr, "ERROR: Could not update index\n");
    return 1;
  }

  // Read old index by line
  char line[FILENAME_SIZE];
  while (fgets(line, sizeof(line), fold_index)) {
    strtok(line, "\n");

    if (strcmp(line, filename) != 0)
      fprintf(fnew_index, "%s\n", line);
  }
  fclose(fold_index);
  fclose(fnew_index);

  fs_mv(".beargit/.new_index", ".beargit/.index");

  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
  const char* correct_msg = go_bears;

  while (*correct_msg != '\0') {
    if (*msg == '\0' || *msg++ != *correct_msg++)
      return 0;
  }

  return 1;
}

void next_commit_id(char* commit_id) {
  char valid_chars[3] = {'6', '1', 'c'};
  srand(time(NULL));

  for (int i = 0; i < COMMIT_ID_BYTES; i++) {
    int rand_idx = rand() % 3;
    commit_id[i] = valid_chars[rand_idx];
  }

  commit_id[COMMIT_ID_BYTES] = '\0';
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  char new_directory_name[FILENAME_SIZE];
  char new_index[60];
  char new_prev[60];
  char new_msg_filename[FILENAME_SIZE];
  sprintf(new_directory_name, ".beargit/%s", commit_id);
  sprintf(new_index, ".beargit/%s/.index", commit_id);
  sprintf(new_prev, ".beargit/%s/.prev", commit_id);
  sprintf(new_msg_filename, ".beargit/%s/.msg", commit_id);

  fs_mkdir(new_directory_name);

  FILE* fnew_index = fopen(new_index, "r");
  FILE* fnew_prev = fopen(new_prev, "r");
  FILE* fnew_msg = fopen(new_msg_filename, "r");

  fs_cp(".beargit/.index", new_index);
  fs_cp(".beargit/.prev", new_prev);
  write_string_to_file(new_msg_filename, msg);

  FILE* findex;
  if ((findex= fopen(".beargit/.index", "r")) == NULL) {
    fprintf(stderr, "ERROR: Could not locate index file\n");
  }

  char file[FILENAME_SIZE];
  while (fgets(file, sizeof(file), findex)) {
    strtok(file, "\n");
    char new_file[FILENAME_SIZE];
    sprintf(new_file, ".beargit/%s/%s", commit_id, file);
    FILE* fnew_file = fopen(new_file, "r");
    fs_cp(file, new_file);
    fclose(fnew_file);
  }

  write_string_to_file(".beargit/.prev", commit_id);

  fclose(fnew_index);
  fclose(fnew_prev);
  fclose(fnew_msg);

  return 0;
}

/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {
  printf("Tracked files:\n\n");

  FILE* findex;
  if ((findex= fopen(".beargit/.index", "r")) == NULL) {
    fprintf(stderr, "ERROR: Could not locate index file\n");
  }

  int files_tracked = 0;
  char line[FILENAME_SIZE];
  while (fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    printf("  %s\n", line);
    files_tracked++;
  }

  printf("\n%d files total\n", files_tracked);

  fclose(findex);

  return 0;
}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

int beargit_log() {

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  char* last_commit_id = commit_id;

  if (strcmp(last_commit_id, zero_commit_id) == 0) {
    fprintf(stderr, "ERROR: There are no commits!\n");
    return 1;
  }

  fprintf(stdout, "\n");

  while (strcmp(last_commit_id, zero_commit_id) != 0) {
    fprintf(stdout, "commit %s\n", last_commit_id);

    char msg_filename[FILENAME_SIZE];
    char msg[MSG_SIZE];
    sprintf(msg_filename, ".beargit/%s/.msg", last_commit_id);
    read_string_from_file(msg_filename, msg, MSG_SIZE);
    fprintf(stdout, "  %s\n", msg);

    char next_prev_filename[FILENAME_SIZE];
    sprintf(next_prev_filename, ".beargit/%s/.prev", last_commit_id);
    char next_commit_id[COMMIT_ID_SIZE];
    read_string_from_file(next_prev_filename, next_commit_id, COMMIT_ID_SIZE);
    last_commit_id = next_commit_id;
    fprintf(stdout, "\n");
  }

  return 0;
}
