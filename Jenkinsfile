def branch_name_filter()
{
   return BRANCH_NAME == "develop" || BRANCH_NAME ==~ /^PR-\d+-merge$/
}

pipeline {

  agent none

  stages {

    stage('Basic Checks') {
      agent {
        docker {
          image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
          alwaysPull true
        }
      }

      stages {
        stage('License Checks') {
          steps {
            sh './scripts/check_license_header.py'
          }
        }
        stage('Style check') {
          steps {
            sh './scripts/apply_style.py -w -a'
          }
        }
        stage('CMake Version Check') {
          steps {
            sh './scripts/check-cmake-versions.py'
          }
        }
      }
    } // basic checks

    stage('Builds & Tests') {
      parallel {

        stage('Static Analysis') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
              alwaysPull true
            }
          }
          steps {
            sh 'mkdir -p build-analysis && cd build-analysis && cmake ../'
            sh './scripts/run_static_analysis.py build-analysis/'
          }
        } // static analysis

        stage('Clang 6 Debug') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
              alwaysPull true
            }
          }

          stages {
            stage('Clang 6 Debug Build') {
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -B Debug'
              }
            }

            stage('Clang 6 Debug Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Debug'
              }
            }

            stage('Clang 6 Debug Integration Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci-tool.py -I Debug'
              }
            }

            stage('Clang 6 Debug End-to-end Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci-tool.py -E Debug'
              }
            }
          }
        } // clang 6 debug

        stage('Clang 6 Release') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
              alwaysPull true
            }
          }

          stages {
            stage('Clang 6 Release Build') {
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -B Release'
              }
            }

            stage('Clang 6 Release Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Release'
              }
            }

            stage('Clang 6 Release Integration Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -I Release'
              }
            }

            stage('Clang 6 Release End-to-end Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci-tool.py -E Release'
              }
            }
          }
        } // clang 6 release

        stage('GCC 7 Debug') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
              alwaysPull true
            }
          }

          environment {
            CC  = 'gcc'
            CXX = 'g++'
          }

          stages {
            stage('GCC 7 Debug Build') {
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -B Debug'
              }
            }

            stage('GCC 7 Debug Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Debug'
              }
            }

            stage('GCC 7 Debug Integration Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -I Debug'
              }
            }

            stage('GCC 7 Debug End-to-end Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci-tool.py -E Debug'
              }
            }
          }
        } // gcc 7 debug

        stage('GCC 7 Release') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
              alwaysPull true
            }
          }

          environment {
            CC  = 'gcc'
            CXX = 'g++'
          }

          stages {
            stage('GCC 7 Release Build') {
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -B Release'
              }
            }

            stage('GCC 7 Release Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Release'
              }
            }

            stage('GCC 7 Release Integration Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci/install-test-dependencies.sh'
                sh './scripts/ci-tool.py -I Release'
              }
            }

            stage('GCC 7 Release End-to-end Tests') {
              when {
                expression {
                  branch_name_filter()
                }
              }
              steps {
                sh './scripts/ci-tool.py -E Release'
              }
            }
          }
        } // gcc 7 release

      } // parallel

    } // build & test

  } // stages

} // pipeline
