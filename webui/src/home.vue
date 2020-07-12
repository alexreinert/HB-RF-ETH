<template>
  <div>
    <sys-info></sys-info>
  </div>
</template>

<script>
import Vue from "vue";

import SysInfo from "./sysinfo.vue";

export default {
  name: "Home",
  data() {
    return {
      showAbout: false
    };
  },
  components: {
    SysInfo
  }
};
</script>

<style lang="css">
</style>