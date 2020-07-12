<template>
  <div id="app" class="container" style="max-width: 600px">
    <Header></Header>
    <router-view></router-view>
  </div>
</template>

<script>
import Header from "./header.vue";

export default {
  name: "App",
  components: {
    Header
  }
};
</script>