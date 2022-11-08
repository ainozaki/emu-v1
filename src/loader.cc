#include "loader.h"

#include <algorithm>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

Loader::Loader(int argc, char **argv, char **envp)
    : filename_(argv[1]), argc_(argc), argv_(argv), envp_(envp) {}

Loader::~Loader() {
  close(fd_);
  munmap(file_map_start_, sb_.st_size);
}

namespace {

uint64_t PAGE_ROUNDUP(uint64_t v) {
  return (v + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

} // namespace

int Loader::init() {
  printf("Loading %s\n", filename_);

  // open ELF file
  fd_ = open(filename_, O_RDWR);
  if (!fd_) {
    fprintf(stderr, "Cannot open %s\n", filename_);
    return EFAILED;
  }
  fstat(fd_, &sb_);

  // make alias for ELF headers
  if ((file_map_start_ = (char *)mmap(NULL, sb_.st_size, PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE, fd_, 0)) == (char *)-1) {
    perror("mmap");
    return EFAILED;
  }
  printf("\tfile_map_start: %p\n", file_map_start_);
  eh_ = (Elf64_Ehdr *)file_map_start_;
  ph_tbl_ = (Elf64_Phdr *)((uint64_t)file_map_start_ + eh_->e_phoff);
  sh_tbl_ = (Elf64_Shdr *)((uint64_t)file_map_start_ + eh_->e_shoff);
  sh_name_ =
      (char *)((uint64_t)file_map_start_ + sh_tbl_[eh_->e_shstrndx].sh_addr);
  return ESUCCESS;
}

int Loader::load() {
  bool map_done;

  // ELF check
  // TODO

  // interp check
  // This emulator can load only static linked binary.
  const char *interp_str = get_interp();
  if (interp_str) {
    printf("cannot load dynamic linked program\n");
    return EFAILED;
  }

  // load segment
  Elf64_Phdr *ph;
  for (int i = 0; i < eh_->e_phnum; i++) {
    ph = &ph_tbl_[i];

    if (ph->p_type != PT_LOAD) {
      continue;
    }

    if (ph->p_vaddr != ph->p_paddr) {
      use_paddr_ = true;
    }

    if (!map_done) {
      // allocate contiguous memory space.
      void *addr;
      text_start = get_text_start_addr();
      text_size = get_text_total_size();
      printf("\ttext_start:0x%lx, text_size:0x%lx\n", text_start, text_size);
      if ((addr = mmap(NULL, PAGE_ROUNDUP(text_size),
                       PROT_READ | PROT_EXEC | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (void *)-1) {
        perror("mmap");
        return EFAILED;
      }
      map_base = (uint64_t)addr;
      map_done = true;
      printf("\tmap_base_addr: 0x%lx\n", map_base);
    }

    // memcpy LOAD segment
    printf("\tmemcpy:\n");
    uint64_t start = use_paddr_ ? ph->p_paddr : ph->p_paddr;
    printf("\t\temu : 0x%lx-0x%lx, size:0x%lx\n", start, start + ph->p_memsz,
           ph->p_memsz);
    memcpy((void *)(map_base + ph->p_paddr - text_start),
           (void *)((uint64_t)file_map_start_ + ph->p_offset), ph->p_memsz);

    // zero clear .bss section
    if (ph->p_memsz > ph->p_filesz) {
      printf("\t\tzero clear .bss: from:0x%lx, size:0x%lx\n",
             map_base + ph->p_paddr + ph->p_filesz - text_start,
             ph->p_memsz - ph->p_filesz);
      memset((void *)(map_base + ph->p_paddr + ph->p_filesz - text_start), 0,
             ph->p_memsz - ph->p_filesz);
    }
    entry = eh_->e_entry - ph->p_vaddr + ph->p_paddr;
  }

  // prepare stack
  uint64_t stack_tp = (uint64_t)calloc(STACK_SIZE, sizeof(char *));
  sp_alloc_start = stack_tp;
  stack_tp = stack_tp - (stack_tp % 16) + 16;
  init_sp = stack_tp + STACK_SIZE - 16 * 4 * 200;
  printf("stack: 0x%lx - 0x%lx, init_sp: 0x%lx\n", stack_tp,
         stack_tp + STACK_SIZE, init_sp);
  return ESUCCESS;
}

const char *Loader::get_interp() const {
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_INTERP) {
      return file_map_start_ + ph_tbl_[i].p_offset;
    }
  }
  return NULL;
}

uint64_t Loader::get_text_total_size() const {
  uint64_t min_addr = (uint64_t)-1;
  uint64_t max_addr = 0;
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_LOAD) {
      if (use_paddr_) {
        min_addr = std::min(ph_tbl_[i].p_paddr, min_addr);
        max_addr = std::max(ph_tbl_[i].p_paddr + ph_tbl_[i].p_memsz, max_addr);
      } else {
        min_addr = std::min(ph_tbl_[i].p_vaddr, min_addr);
        max_addr = std::max(ph_tbl_[i].p_vaddr + ph_tbl_[i].p_memsz, max_addr);
      }
    }
  }
  return max_addr - min_addr;
}

uint64_t Loader::get_text_start_addr() const {
  uint64_t min_addr = (uint64_t)-1;
  for (int i = 0; i < eh_->e_phnum; i++) {
    if (ph_tbl_[i].p_type == PT_LOAD) {
      min_addr = use_paddr_ ? std::min(ph_tbl_[i].p_paddr, min_addr)
                            : std::min(ph_tbl_[i].p_vaddr, min_addr);
    }
  }
  return min_addr;
}
