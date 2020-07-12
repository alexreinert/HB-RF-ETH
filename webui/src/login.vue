<template>
  <b-card
    :header="$t('title')"
    header-tag="h6"
    header-bg-variant="secondary"
    header-text-variant="white"
    class="mb-3"
  >
    <b-form @submit.stop.prevent>
      <b-form-group :label="$t('password')" label-cols-sm="4">
        <b-form-input
          type="password"
          v-model="$v.password.$model"
          :state="validateState('password')"
        ></b-form-input>
      </b-form-group>
      <b-alert
        variant="danger"
        :show="showError"
        dismissible
        fade
        @dismissed="showError=null"
      >{{ $t('loginError') }}</b-alert>
      <b-form-group label-cols-sm="9">
        <b-button
          variant="primary"
          block
          @click="loginClick"
          :disabled="password==null||password==''"
        >{{ $t('login') }}</b-button>
      </b-form-group>
    </b-form>
  </b-card>
</template>

<script>
import Vue from "vue";

import Vuelidate from "vuelidate";
Vue.use(Vuelidate);
import { required } from "vuelidate/lib/validators";

import VueI18n from "vue-i18n";
Vue.use(VueI18n);

import { LayoutPlugin } from "bootstrap-vue/esm/components/layout";
Vue.use(LayoutPlugin);

import { FormPlugin } from "bootstrap-vue/esm/components/form";
Vue.use(FormPlugin);

import { FormGroupPlugin } from "bootstrap-vue/esm/components/form-group";
Vue.use(FormGroupPlugin);

import { FormInputPlugin } from "bootstrap-vue/esm/components/form-input";
Vue.use(FormInputPlugin);

import { ButtonPlugin } from "bootstrap-vue/esm/components/button";
Vue.use(ButtonPlugin);

import { CardPlugin } from "bootstrap-vue/esm/components/card";
Vue.use(CardPlugin);

import { AlertPlugin } from "bootstrap-vue/esm/components/alert";
Vue.use(AlertPlugin);

export default {
  name: "Login",
  data() {
    return {
      password: "",
      showError: null
    };
  },
  validations: {
    password: {
      required
    }
  },
  methods: {
    validateState(name) {
      const { $dirty, $error } = this.$v[name];
      return $dirty && $error ? false : null;
    },
    loginClick(event) {
      var self = this;
      this.showError = null;
      this.$store.dispatch("login/tryLogin", this.password).then(
        () => {
          self.$router.push(self.$route.query.redirect || "/");
        },
        () => {
          self.showError = 10;
        }
      );
    }
  },
  i18n: {
    locale: navigator.language,
    fallbackLocale: "en",
    messages: {
      de: {
        title: "Bitte anmelden",
        password: "Passwort",
        login: "Anmelden",
        loginError: "Anmelden war nicht erfolgreich."
      },
      en: {
        title: "Please log in",
        password: "Password",
        login: "Login",
        loginError: "Login was not successful."
      }
    }
  }
};
</script>

<style lang="css">
</style>