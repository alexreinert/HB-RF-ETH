Vue.use(BootstrapVue);
Vue.use(window.vuelidate.default);

var required = validators.required;
var requiredIf = validators.requiredIf;
var requiredUnless = validators.requiredUnless;
var minLength = validators.minLength;
var maxLength = validators.maxLength;
var ipAddress = validators.ipAddress;
var sameAs = validators.sameAs;
var numeric = validators.numeric;

var app = new Vue({
    el: '#app',
    data: {
        login: {
            isLoggedIn: false,
            password: "",
            error: null
        },
        settings: {
            adminPassword: "",
            adminPasswordRepeat: "",
            hostname: "",
            useDHCP: true,
            localIP: "",
            netmask: "",
            gateway: "",
            dns1: "",
            dns2: "",
            timesource: 0,
            dcfOffset: 0,
            gpsBaudrate: 9600,
            ntpServer: "",

            success: null,
            error: null,
        },
        firmware: {
            file: null,
            progress: 0,
            success: null,
            error: null,
        },
        sysInfo: {
            currentVersion: "",
            latestVersion: ""
        }
    },
    validations: {
        login: {
            password: {
                required
            }
        },
        settings: {
            adminPassword: {
                minLength: minLength(5),
                maxLength: maxLength(32),
            },
            adminPasswordRepeat: {
                sameAsPassword: sameAs('adminPassword'),
            },
            hostname: {
                required,
                maxLength: maxLength(32)
            },
            localIP: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            netmask: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            gateway: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            dns1: {
                required: requiredUnless('useDHCP'),
                ipAddress
            },
            dns2: {
                ipAddress
            },
            ntpServer: {
                required: requiredIf('isNtpActived'),
            },
            dcfOffset: {
                required: requiredIf('isDcfActived'),
                numeric
            }
        }
    },
    computed: {
        isNtpActivated: function () {
            return this.settings.timesource == 0;
        },
        isDcfActivated: function () {
            return this.settings.timesource == 1;
        },
        isGpsActivated: function () {
            return this.settings.timesource == 2;
        }
    },
    methods: {
        validateState(group, name) {
            const { $dirty, $error } = this.$v[group][name];
            return $dirty && $error ? false : null;
        },
        loginClick: function (event) {
            var self = this;
            self.login.error = null;
            $.ajax({
                url: "getData.json",
                type: 'GET',
                cache: false,
                dataType: "json",
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (json) {
                    if (json.loggedIn) {
                        self.login.isLoggedIn = true;
                        self.sysInfo = json.sysInfo;
                        self.settings = json.settings;
                        self.$v.settings.$touch();
                    } else {
                        self.login.error = "Login was not successful"
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.login.error = "Login was not successful"
                }
            });
        },
        firmwareUpdateClick: function (event) {
            var self = this;

            self.firmware.success = null;
            self.firmware.error = null;
            self.firmware.progress = 0;

            var form = new FormData();
            form.append("file", self.firmware.file, self.firmware.file.name);
            form.append("upload_file", true);

            $.ajax({
                type: "POST",
                url: "/ota_update",
                xhr: function () {
                    var myXhr = $.ajaxSettings.xhr();
                    if (myXhr.upload) {
                        myXhr.upload.addEventListener('progress',
                            function (event) {
                                var pos = event.loaded || event.position;
                                var tot = event.total;
                                if (event.lengthComputable) {
                                    self.firmware.progress = Math.ceil(pos / tot * 100);
                                }
                            },
                            false);
                    }
                    return myXhr;
                },
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (data) {
                    self.firmware.success = data;
                    self.firmware.progress = 0;
                    self.firmware.file = null;
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.firmware.progress = 0;
                    self.firmware.error = "An error occured";
                },
                async: true,
                data: form,
                cache: false,
                contentType: false,
                processData: false,
                timeout: 60000
            });
        },
        saveSettingsClick: function (event) {
            var self = this;

            this.$v.settings.$touch();
            if (this.$v.settings.$anyError) return;

            self.settings.error = null;
            self.settings.success = null;

            $.ajax({
                url: "settings.json",
                type: 'POST',
                cache: false,
                dataType: "json",
                data: JSON.stringify(self.settings),
                headers: { "Authorization": "Basic " + btoa("admin:" + self.login.password) },
                success: function (json) {
                    self.settings = json.settings;
                    self.settings.success = "Settings were successfully saved. Please restart to take them effect."
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    self.settings.error = "An error occurred."
                },
                complete: function (jqXHR, textStatus) {
                    self.settings.adminPasswordRepeat = "";
                    self.settings.adminPassword = "";
                }
            });
        }
    }
});
